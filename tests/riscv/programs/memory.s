# Programa 4: Acesso à Memória
# memory.s - Demonstra load/store de dados
#
# Saída esperada: x3 = 123 (valor lido da memória)
#                x5 = 999 (soma de dois valores da memória)
#
# Como executar no Hidra:
# 1. Carregue na memória em 0x0
# 2. Execute step-by-step para ver load/stores
# 3. Verifique registradores e conteúdo da memória

    .text
    .globl _start

_start:
    # Setup de ponteiros e valores
    ADDI x1, x0, 100    # x1 = 100 (endereço base na memória)
    ADDI x2, x0, 42     # x2 = 42 (primeiro valor)
    ADDI x4, x0, 87     # x4 = 87 (segundo valor)
    
    # Store: mem[100] = 42
    SW x2, 0(x1)        # mem[x1+0] = x2
    
    # Store: mem[104] = 87
    SW x4, 4(x1)        # mem[x1+4] = x4
    
    # Load: x3 = mem[100]
    LW x3, 0(x1)        # x3 = mem[x1+0] = 42
    
    # Load: x5 = mem[104]
    LW x5, 4(x1)        # x5 = mem[x1+4] = 87
    
    # Soma os valores carregados
    ADD x5, x3, x5      # x5 = x3 + x5 = 42 + 87 = 129
    
    # Teste com halfword (16 bits)
    ADDI x6, x0, 200    # x6 = 200 (novo endereço)
    ADDI x7, x0, 0xABCD # x7 = 0xABCD (valor 16 bits)
    
    # Store halfword
    SH x7, 0(x6)        # mem[200] = 0xABCD (apenas 16 bits)
    
    # Load halfword com sign extension
    LH x8, 0(x6)        # x8 = mem[200] com sign extend
    
    # Teste com byte (8 bits)
    ADDI x9, x0, 300    # x9 = 300 (endereço)
    ADDI x10, x0, 0x5A  # x10 = 0x5A (valor 8 bits)
    
    # Store byte
    SB x10, 0(x9)       # mem[300] = 0x5A (apenas 8 bits)
    
    # Load byte com sign extension
    LB x11, 0(x9)       # x11 = mem[300] com sign extend
    
fim:
    # Fim do programa
    JAL x0, fim         # Loop infinito

# ============================================================
# Expected Output:
# ============================================================
# Registrador x1: 100 (0x64) - endereço base
# Registrador x2: 42 (0x2A) - primeiro valor
# Registrador x3: 42 (0x2A) <- Load bem-sucedido!
# Registrador x4: 87 (0x57) - segundo valor
# Registrador x5: 129 (0x81) <- Soma dos loads: 42+87
# 
# Memória:
#   mem[100] = 42 (0x0000002A)
#   mem[104] = 87 (0x00000057)
#   mem[200] = 0xABCD (0x0000ABCD)
#   mem[300] = 0x5A (0x0000005A)
#
# Registrador x6: 200 (0xC8) - endereço halfword
# Registrador x7: 43981 (0xABCD) - valor halfword
# Registrador x8: 43981 (0xABCD) ou -21555 (signed) <- LH
# Registrador x9: 300 (0x12C) - endereço byte
# Registrador x10: 90 (0x5A) - valor byte
# Registrador x11: 90 (0x5A) <- LB
# ============================================================
