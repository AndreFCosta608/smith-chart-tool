import os
import torch
import numpy as np
import pandas as pd
import torch.nn as nn

# 1. Gerador de Dados Sintéticos com Parasitas Comerciais
def generate_synthetic_dataset(num_samples=10000):
    Z0 = 50.0
    frequencies = np.random.uniform(100e6, 3e9, num_samples) # 100MHz a 3GHz
    R_load = np.random.uniform(5.0, 300.0, num_samples)
    X_load = np.random.uniform(-300.0, 300.0, num_samples)
    
    # Valores base da Série E24
    e24_base = np.array([1.0, 1.1, 1.2, 1.3, 1.5, 1.6, 1.8, 2.0, 2.2, 2.4, 2.7, 3.0, 
                         3.3, 3.6, 3.9, 4.3, 4.7, 5.1, 5.6, 6.2, 6.8, 7.5, 8.2, 9.1])
    
    L_commercial = np.unique((e24_base[:, None] * (10.0 ** np.array([-9, -8, -7]))).flatten())
    C_commercial = np.unique((e24_base[:, None] * (10.0 ** np.array([-12, -11, -10]))).flatten())
    
    data = []
    for i in range(num_samples):
        f, R, X = frequencies[i], R_load[i], X_load[i]
        omega = 2 * np.pi * f
        topology = 0 if R >= Z0 else 1
        
        # Equações exatas de casamento de impedância
        if topology == 0:
            B_ideal = np.sqrt((R/Z0 - 1) / (R**2 + X**2)) if R > Z0 else 0.0
            X_ideal = -X + np.sqrt(Z0*(R - Z0)) if R > Z0 else 0.0
        else:
            X_ideal = np.sqrt(R*(Z0 - R)) - X if Z0 > R else 0.0
            B_ideal = np.sqrt((Z0 - R) / R) / Z0 if Z0 > R else 0.0
            
        L_val = max(1e-9, X_ideal / omega) if omega > 0 else 1e-9
        C_val = max(1e-12, B_ideal / omega) if omega > 0 else 1e-12
        
        # Associação ao catálogo comercial
        idx_L = np.argmin(np.abs(L_commercial - L_val))
        idx_C = np.argmin(np.abs(C_commercial - C_val))
        
        # Simulação de Parasitas (ESR/ESL)
        ESR_L = 0.05 * np.sqrt(L_commercial[idx_L] * 1e9)
        ESR_C = 0.01 + 0.001 * (1e12 / (C_commercial[idx_C] * 1e12 + 1e-6))
        
        data.append({
            'f_Hz': f, 'R_load': R, 'X_load': X,
            'topology': topology,
            'L_ideal_nH': L_val * 1e9, 'C_ideal_pF': C_val * 1e12,
            'L_comm_idx': idx_L, 'C_comm_idx': idx_C,
            'ESR_L': ESR_L, 'ESR_C': ESR_C
        })
        
    return pd.DataFrame(data)

# 2. Definição da Rede Multi-Task (Modelo C)
class MultiTaskMatchingNet(nn.Module):
    def __init__(self, num_L_classes=72, num_C_classes=72):
        super().__init__()
        self.shared = nn.Sequential(
            nn.Linear(3, 128),
            nn.BatchNorm1d(128),
            nn.ReLU(),
            nn.Linear(128, 128),
            nn.ReLU()
        )
        self.head_topology = nn.Linear(128, 2)
        self.head_L = nn.Linear(128, num_L_classes)
        self.head_C = nn.Linear(128, num_C_classes)
        
    def forward(self, x):
        feat = self.shared(x)
        return self.head_topology(feat), self.head_L(feat), self.head_C(feat)


if __name__ == "__main__":
    # Salva sempre no mesmo diretório onde o script está localizado,
    # independente de qual seja o diretório de trabalho (cwd) no momento da execução.
    script_dir = os.path.dirname(os.path.abspath(__file__))
    output_path = os.path.join(script_dir, "synthetic_matching_dataset.csv")

    print("Gerando dataset sintético...")
    df = generate_synthetic_dataset(num_samples=600000)

    df.to_csv(output_path, index=False)

    print(f"Dataset gerado com {len(df)} amostras.")
    print(f"Salvo em: {output_path}")
    print(df.head())
