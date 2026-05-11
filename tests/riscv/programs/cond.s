# Programa 3: Jump Condicional
# cond.s - Demonstra branches condicionais
#
# Saída esperada: x4 = 1 (branch tomado)
#                x6 = 1 (branch não tomado)
#
# Como executar no Hidra:
# 1. Carregue na memória em 0x0
# 2. Execute step-by-step para ver branches
# 3. Verifique registradores de resultado

    .text
    .globl _start

_start:
    # Setup inicial
    ADDI x1, x0, 10     # x1 = 10
    ADDI x2, x0, 20     # x2 = 20
    ADDI x3, x0, 10     # x3 = 10 (igual a x1)
    
    # Teste 1: BEQ (Branch if Equal)
    # x1 == x3, então branch é tomado
    BEQ x1, x3, teste1_passa
    ADDI x4, x0, 0      # Não executado (se chegasse aqui)
    JAL x0, teste2
teste1_passa:
    ADDI x4, x0, 1      # x4 = 1 (BEQ funcionou!)
    
teste2:
    # Teste 2: BNE (Branch if Not Equal)
    # x1 != x2, então branch é tomado
    BNE x1, x2, teste2_passa
    ADDI x5, x0, 0      # Não executado
    JAL x0, teste3
teste2_passa:
    ADDI x5, x0, 1      # x5 = 1 (BNE funcionou!)
    
teste3:
    # Teste 3: BLT (Branch if Less Than)
    # x1 < x2 (10 < 20), então branch é tomado
    BLT x1, x2, teste3_passa
    ADDI x6, x0, 0      # Não executado
    JAL x0, teste4
teste3_passa:
    ADDI x6, x0, 1      # x6 = 1 (BLT funcionou!)
    
teste4:
    # Teste 4: BGE (Branch if Greater or Equal)
    # x2 >= x1 (20 >= 10), então branch é tomado
    BGE x2, x1, teste4_passa
    ADDI x7, x0, 0      # Não executado
    JAL x0, fim
teste4_passa:
    ADDI x7, x0, 1      # x7 = 1 (BGE funcionou!)
    
fim:
    # Fim do programa
    JAL x0, fim         # Loop infinito

# ============================================================
# Expected Output:
# ============================================================
# Registrador x1: 10 (0xA)
# Registrador x2: 20 (0x14)
# Registrador x3: 10 (0xA)
# Registrador x4: 1  (0x1) <- BEQ tomado
# Registrador x5: 1  (0x1) <- BNE tomado
# Registrador x6: 1  (0x1) <- BLT tomado
# Registrador x7: 1  (0x1) <- BGE tomado
# PC final: endereço do loop infinito
# ============================================================
# Todos os branches foram tomados corretamente!
# ============================================================
