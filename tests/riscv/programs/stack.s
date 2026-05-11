# Programa 5: Operações com Stack
# stack.s - Demonstra operações de push/pop na stack
#
# Saída esperada: x4 = 123, x6 = 456 (valores recuperados da stack)
#
# Como executar no Hidra:
# 1. Carregue na memória em 0x0
# 2. Execute step-by-step para ver operações de stack
# 3. Verifique registradores e memória da stack

    .text
    .globl _start

_start:
    # Inicializa stack pointer (sp = x2)
    # Em RISC-V, stack cresce para endereços menores
    ADDI x2, x0, 0x1000  # sp = 0x1000 (topo da stack)
    
    # ========================================
    # PUSH: Empilha dois valores
    # ========================================
    
    # Prepara valores
    ADDI x1, x0, 123     # x1 = 123 (primeiro valor)
    ADDI x3, x0, 456     # x3 = 456 (segundo valor)
    
    # Push x1 na stack
    ADDI x2, x2, -4      # sp = sp - 4 (aloca espaço)
    SW x1, 0(x2)         # mem[sp] = x1
    
    # Push x3 na stack
    ADDI x2, x2, -4      # sp = sp - 4
    SW x3, 0(x2)         # mem[sp] = x3
    
    # ========================================
    # POP: Recupera valores da stack
    # ========================================
    
    # Pop primeiro valor (deve ser 456 - LIFO)
    LW x4, 0(x2)         # x4 = mem[sp]
    ADDI x2, x2, 4       # sp = sp + 4 (desaloca)
    
    # Pop segundo valor (deve ser 123)
    LW x6, 0(x2)         # x6 = mem[sp]
    ADDI x2, x2, 4       # sp = sp + 4
    
    # ========================================
    # Verifica se stack voltou ao original
    # ========================================
    # sp deve ser 0x1000 novamente
    
    # ========================================
    # Teste adicional: salva e restaura registrador
    # ========================================
    
    ADDI x7, x0, 999     # x7 = 999 (valor importante)
    
    # Salva x7 na stack antes de "chamada de função"
    ADDI x2, x2, -4
    SW x7, 0(x2)
    
    # Simula operação que modifica x7
    ADDI x7, x0, 0       # x7 = 0 (modificado!)
    
    # Restaura x7 da stack
    LW x7, 0(x2)
    ADDI x2, x2, 4
    
    # x7 deve ser 999 novamente!
    
fim:
    # Fim do programa
    JAL x0, fim          # Loop infinito

# ============================================================
# Expected Output:
# ============================================================
# Registrador x1: 123 (0x7B) - primeiro valor pushado
# Registrador x2: 0x1000 - stack pointer (voltou ao original)
# Registrador x3: 456 (0x1C8) - segundo valor pushado
# Registrador x4: 456 (0x1C8) <- Primeiro pop (LIFO)
# Registrador x6: 123 (0x7B) <- Segundo pop
# Registrador x7: 999 (0x3E7) <- Valor restaurado da stack
#
# Memória da stack (endereço 0x1000):
#   Antes: vazia
#   Após pushes: 
#     mem[0xFFC] = 456 (topo)
#     mem[0xFF8] = 123
#   Após pops: vazia novamente
# ============================================================
# Nota: Stack em RISC-V cresce para endereços decrescentes!
# ============================================================
