# Programa 1: Soma Simples
# soma.s - Calcula a soma de dois números
#
# Saída esperada: x3 = 75 (25 + 50)
#
# Como executar no Hidra:
# 1. Carregue este programa na memória starting em 0x0
# 2. Execute step-by-step ou run
# 3. Verifique que x3 contém 75

    .text
    .globl _start

_start:
    # Inicializa operandos
    ADDI x1, x0, 25     # x1 = 25 (primeiro operando)
    ADDI x2, x0, 50     # x2 = 50 (segundo operando)
    
    # Realiza soma
    ADD x3, x1, x2      # x3 = x1 + x2 = 75
    
    # Fim do programa (loop infinito)
fim:
    JAL x0, fim         # Jump para fim (loop infinito)

    # Dados (opcional, se usar load/store)
    .data
valor1: .word 25
valor2: .word 50

# ============================================================
# Expected Output:
# ============================================================
# Registrador x1: 25 (0x19)
# Registrador x2: 50 (0x32)
# Registrador x3: 75 (0x4B) <- RESULTADO
# PC final: endereço do loop infinito
# ============================================================
