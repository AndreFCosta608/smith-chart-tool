import os
import torch
import numpy as np
import pandas as pd
import torch.nn as nn
import torch.optim as optim
from torch.utils.data import Dataset, DataLoader

# Configuração de dispositivo (GPU se disponível, senão CPU)
device = torch.device("cuda" if torch.cuda.is_available() else "cpu")
print(f"Utilizando dispositivo: {device}")

# ==============================================================================
# 1. CARREGAMENTO E NORMALIZAÇÃO DOS DADOS
# ==============================================================================
class SmithMatchingDataset(Dataset):
    def __init__(self, csv_file="synthetic_matching_dataset.csv"):
        df = pd.read_csv(csv_file)
        
        # Inputs: [f_Hz, R_load, X_load]
        raw_X = df[['f_Hz', 'R_load', 'X_load']].values.astype(np.float32)
        
        # Normalização Min-Max para estabilidade do gradiente
        self.mean_std = {
            'f_max': 3e9,
            'R_max': 300.0,
            'X_max': 300.0
        }
        
        raw_X[:, 0] /= self.mean_std['f_max']
        raw_X[:, 1] /= self.mean_std['R_max']
        raw_X[:, 2] /= self.mean_std['X_max']
        
        self.X = torch.tensor(raw_X, dtype=torch.float32)
        self.y_topo = torch.tensor(df['topology'].values, dtype=torch.long)
        self.y_L_ideal = torch.tensor(df['L_ideal_nH'].values, dtype=torch.float32).unsqueeze(1)
        self.y_C_ideal = torch.tensor(df['C_ideal_pF'].values, dtype=torch.float32).unsqueeze(1)
        self.y_L_idx = torch.tensor(df['L_comm_idx'].values, dtype=torch.long)
        self.y_C_idx = torch.tensor(df['C_comm_idx'].values, dtype=torch.long)

    def __len__(self):
        return len(self.X)

    def __getitem__(self, idx):
        return {
            'X': self.X[idx],
            'topo': self.y_topo[idx],
            'L_ideal': self.y_L_ideal[idx],
            'C_ideal': self.y_C_ideal[idx],
            'L_idx': self.y_L_idx[idx],
            'C_idx': self.y_C_idx[idx]
        }


# ==============================================================================
# 2. MODELO A: SURROGATE MLP (Aproximação Contínua Ultrarrápida)
# ==============================================================================
class SurrogateMLP(nn.Module):
    def __init__(self):
        super(SurrogateMLP, self).__init__()
        self.net = nn.Sequential(
            nn.Linear(3, 64),
            nn.ReLU(),
            nn.Linear(64, 128),
            nn.ReLU(),
            nn.Linear(128, 64),
            nn.ReLU(),
            nn.Linear(64, 3)  # [Topologia_logit, L_ideal_nH, C_ideal_pF]
        )
        
    def forward(self, x):
        return self.net(x)

def train_model_a(dataloader, epochs=50):
    print("\n--- Treinando Modelo A (Surrogate MLP) ---")
    model = SurrogateMLP().to(device)
    optimizer = optim.Adam(model.parameters(), lr=0.001)
    criterion_mse = nn.MSELoss()
    criterion_bce = nn.BCEWithLogitsLoss()
    
    model.train()
    for epoch in range(1, epochs + 1):
        total_loss = 0.0
        for batch in dataloader:
            X = batch['X'].to(device)
            target_topo = batch['topo'].float().unsqueeze(1).to(device)
            target_L = batch['L_ideal'].to(device)
            target_C = batch['C_ideal'].to(device)
            
            optimizer.zero_grad()
            preds = model(X)
            
            loss_topo = criterion_bce(preds[:, 0:1], target_topo)
            loss_L = criterion_mse(preds[:, 1:2], target_L)
            loss_C = criterion_mse(preds[:, 2:3], target_C)
            
            loss = loss_topo + 0.1 * loss_L + 0.1 * loss_C
            loss.backward()
            optimizer.step()
            total_loss += loss.item()
            
        if epoch % 10 == 0:
            print(f"Epoch {epoch}/{epochs} | Loss: {total_loss / len(dataloader):.4f}")
            
    # Exportação para ONNX
    model.eval()
    dummy_input = torch.randn(1, 3, dtype=torch.float32).to(device)
    torch.onnx.export(
        model, dummy_input, "surrogate_model.onnx",
        input_names=['input_features'],
        output_names=['predictions'],
        dynamic_axes={'input_features': {0: 'batch_size'}, 'predictions': {0: 'batch_size'}}
    )
    print("Modelo A salvo e exportado para 'surrogate_model.onnx'!")
    return model


# ==============================================================================
# 3. MODELO C: MULTI-TASK CLASSIFIER (Seleção Direta de Catálogo)
# ==============================================================================
class MultiTaskClassifier(nn.Module):
    def __init__(self, num_L_classes=72, num_C_classes=72):
        super(MultiTaskClassifier, self).__init__()
        self.shared_backbone = nn.Sequential(
            nn.Linear(3, 128),
            nn.BatchNorm1d(128),
            nn.ReLU(),
            nn.Linear(128, 256),
            nn.BatchNorm1d(256),
            nn.ReLU(),
            nn.Linear(256, 128),
            nn.ReLU()
        )
        
        # Cabeças de inferência paralelas
        self.head_topology = nn.Linear(128, 2)              # Classificação: Topologia (0 ou 1)
        self.head_L_catalog = nn.Linear(128, num_L_classes) # Classificação: Ímóvel do Indutor
        self.head_C_catalog = nn.Linear(128, num_C_classes) # Classificação: Ímóvel do Capacitor
        
    def forward(self, x):
        feat = self.shared_backbone(x)
        return self.head_topology(feat), self.head_L_catalog(feat), self.head_C_catalog(feat)

def train_model_c(dataloader, epochs=50, num_L_classes=72, num_C_classes=72):
    print("\n--- Treinando Modelo C (Multi-Task Classifier) ---")
    model = MultiTaskClassifier(num_L_classes, num_C_classes).to(device)
    optimizer = optim.Adam(model.parameters(), lr=0.001)
    criterion_ce = nn.CrossEntropyLoss()
    
    model.train()
    for epoch in range(1, epochs + 1):
        total_loss = 0.0
        for batch in dataloader:
            X = batch['X'].to(device)
            target_topo = batch['topo'].to(device)
            target_L_idx = batch['L_idx'].to(device)
            target_C_idx = batch['C_idx'].to(device)
            
            optimizer.zero_grad()
            out_topo, out_L, out_C = model(X)
            
            loss_topo = criterion_ce(out_topo, target_topo)
            loss_L = criterion_ce(out_L, target_L_idx)
            loss_C = criterion_ce(out_C, target_C_idx)
            
            loss = loss_topo + loss_L + loss_C
            loss.backward()
            optimizer.step()
            total_loss += loss.item()
            
        if epoch % 10 == 0:
            print(f"Epoch {epoch}/{epochs} | Loss: {total_loss / len(dataloader):.4f}")
            
    # Exportação para ONNX
    model.eval()
    dummy_input = torch.randn(1, 3, dtype=torch.float32).to(device)
    torch.onnx.export(
        model, dummy_input, "multitask_model.onnx",
        input_names=['input_features'],
        output_names=['topology_logits', 'L_catalog_logits', 'C_catalog_logits'],
        dynamic_axes={'input_features': {0: 'batch_size'}}
    )
    print("Modelo C salvo e exportado para 'multitask_model.onnx'!")
    return model


# ==============================================================================
# 4. MODELO B: ENVIRONMENT & REDE PPO (Deep Reinforcement Learning)
# ==============================================================================
class SmithMatchingEnv:
    """Ambiente simplificado de RL para síntese interativa de casamento de impedância."""
    def __init__(self, Z0=50.0):
        self.Z0 = Z0
        self.reset()
        
    def reset(self):
        self.f = np.random.uniform(100e6, 3e9)
        self.R = np.random.uniform(5.0, 300.0)
        self.X = np.random.uniform(-300.0, 300.0)
        self.steps = 0
        return self._get_state()
        
    def _get_state(self):
        return np.array([self.f / 3e9, self.R / 300.0, self.X / 300.0], dtype=np.float32)

    def step(self, action):
        # Ações: 0 -> Adicionar L-Série, 1 -> Adicionar C-Paralelo, 2 -> Encerrar
        self.steps += 1
        
        # Recálculo simples da nova impedância após a ação
        if action == 0:
            self.X += 20.0 # Exemplo de adição indutiva
        elif action == 1:
            self.R *= 0.9  # Exemplo de transformação capacitiva
            
        # Recompensa baseada na proximidade de Z0 (50 ohms) e X=0
        reflection = np.sqrt((self.R - self.Z0)**2 + self.X**2)
        reward = -reflection / 50.0
        
        done = (self.steps >= 5) or (reflection < 2.0)
        return self._get_state(), reward, done

class PPOActorCritic(nn.Module):
    def __init__(self, state_dim=3, action_dim=3):
        super(PPOActorCritic, self).__init__()
        # Actor Network (Ações discretas)
        self.actor = nn.Sequential(
            nn.Linear(state_dim, 64),
            nn.Tanh(),
            nn.Linear(64, 64),
            nn.Tanh(),
            nn.Linear(64, action_dim),
            nn.Softmax(dim=-1)
        )
        # Critic Network (Valor do Estado)
        self.critic = nn.Sequential(
            nn.Linear(state_dim, 64),
            nn.Tanh(),
            nn.Linear(64, 64),
            nn.Tanh(),
            nn.Linear(64, 1)
        )

    def forward(self, state):
        return self.actor(state), self.critic(state)

def train_model_b_ppo(episodes=200):
    print("\n--- Treinando Modelo B (Deep RL - PPO Agent) ---")
    env = SmithMatchingEnv()
    policy = PPOActorCritic().to(device)
    optimizer = optim.Adam(policy.parameters(), lr=0.002)
    
    for ep in range(1, episodes + 1):
        state = env.reset()
        done = False
        ep_reward = 0.0
        
        while not done:
            state_t = torch.tensor(state, dtype=torch.float32).unsqueeze(0).to(device)
            probs, value = policy(state_t)
            
            dist = torch.distributions.Categorical(probs)
            action = dist.sample()
            
            next_state, reward, done = env.step(action.item())
            
            # Loss do PPO simplificada para demonstração do loop
            loss_actor = -dist.log_prob(action) * (reward + value.item())
            loss_critic = (value - reward) ** 2
            loss = loss_actor + loss_critic
            
            optimizer.zero_grad()
            loss.backward()
            optimizer.step()
            
            state = next_state
            ep_reward += reward
            
        if ep % 40 == 0:
            print(f"Episódio {ep}/{episodes} | Recompensa Média: {ep_reward:.2f}")

    # Exportação da Política (Actor) para ONNX
    policy.eval()
    dummy_input = torch.randn(1, 3, dtype=torch.float32).to(device)
    torch.onnx.export(
        policy.actor, dummy_input, "ppo_policy_model.onnx",
        input_names=['state'],
        output_names=['action_probabilities'],
        dynamic_axes={'state': {0: 'batch_size'}}
    )
    print("Modelo B (Política RL) salvo e exportado para 'ppo_policy_model.onnx'!")


# ==============================================================================
# 5. EXECUÇÃO PRINCIPAL
# ==============================================================================
if __name__ == "__main__":
    dataset_path = "synthetic_matching_dataset.csv"
    if not os.path.exists(dataset_path):
        raise FileNotFoundError(f"O arquivo {dataset_path} não foi encontrado. Execute o gerador de dados sintéticos primeiro.")

    dataset = SmithMatchingDataset(dataset_path)
    dataloader = DataLoader(dataset, batch_size=64, shuffle=True)
    
    # Executa o treinamento sequencial de todos os 3 modelos
    train_model_a(dataloader, epochs=30)
    train_model_c(dataloader, epochs=30)
    train_model_b_ppo(episodes=100)
    
    print("\n✅ Treinamento concluído! Todos os modelos ONNX foram gerados:")
    print("  1. surrogate_model.onnx")
    print("  2. multitask_model.onnx")
    print("  3. ppo_policy_model.onnx")

