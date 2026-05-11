# Programa 2: Loop com Contador
# loop.s - Conta de 0 a 10 usando loop
#
# Saída esperada: x1 = 10 (contador final)
#                x2 = 55 (soma acumulada: 0+1+2+...+10)
#
# Como executar no Hidra:
# 1. Carregue na memória em 0x0
# 2. Execute até o loop terminar (aproximadamente 30 ciclos)
# 3. Verifique x1=10 e x2=55

    .text
    .globl _start

_start:
    # Inicializa variáveis
    ADDI x1, x0, 0      # x1 = 0 (contador i)
    ADDI x2, x0, 0      # x2 = 0 (soma acumulada)
    ADDI x3, x0, 10     # x3 = 10 (limite do loop)
    
loop_inicio:
    # Verifica condição: se i > 10, sai do loop
    BGT x1, x3, loop_fim    # if (i > 10) break
    
    # soma += i
    ADD x2, x2, x1          # soma = soma + i
    
    # i++
    ADDI x1, x1, 1          # i = i + 1
    
    # Volta para início do loop
    JAL x0, loop_inicio
    
loop_fim:
    # Fim do programa
fim:
    JAL x0, fim             # Loop infinito

# ============================================================
# Expected Output:
# ============================================================
# Registrador x1: 11 (0xB) - contador após sair do loop
# Registrador x2: 55 (0x37) - soma de 0+1+2+...+10 <- RESULTADO
# Registrador x3: 10 (0xA) - limite
# PC final: endereço do loop infinito
# ============================================================
# Nota: O loop executa 11 vezes (i=0 até i=10 inclusive)
# Soma = 0+1+2+3+4+5+6+7+8+9+10 = 55
# ============================================================
