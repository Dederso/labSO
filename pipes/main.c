#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/wait.h>
#include <string.h>
#include <time.h>

#define BUFFER_SIZE 256

int main() {
    int X;
    int pipe_pai_p1[2], pipe_p1_p2[2], pipe_p2_pai[2], pipe_p3_pai[2];
    
    // Passo 1: Ler valor X do teclado
    printf("Digite X (1-5): ");
    scanf("%d", &X);
    if(X < 1 || X > 5) {
        fprintf(stderr, "Valor inválido! X deve estar entre 1 e 5\n");
        exit(EXIT_FAILURE);
    }

    // Criar todos os pipes necessários
    if(pipe(pipe_pai_p1) == -1 || pipe(pipe_p1_p2) == -1 || 
       pipe(pipe_p2_pai) == -1 || pipe(pipe_p3_pai) == -1) {
        perror("Erro ao criar pipes");
        exit(EXIT_FAILURE);
    }

    // Passo 2: Criar processo P1
    pid_t p1 = fork();
    if(p1 == 0) {
        // Processo P1
        close(pipe_pai_p1[1]);  // Fecha extremidade de escrita do pipe P0->P1
        close(pipe_p1_p2[0]);   // Fecha extremidade de leitura do pipe P1->P2

        close(pipe_p2_pai[0]);
        close(pipe_p2_pai[1]);
        close(pipe_p3_pai[0]);
        close(pipe_p3_pai[1]);

        // Passo 5: Receber e imprimir mensagem
        int x_recebido;
        char mensagem[BUFFER_SIZE];
        
        read(pipe_pai_p1[0], &x_recebido, sizeof(int));
        read(pipe_pai_p1[0], mensagem, BUFFER_SIZE);
        close(pipe_pai_p1[0]);
        printf("P1 recebeu: %s\n", mensagem);

        // Passo 6: Gerar array randômico
        srand(time(NULL));
        int tamanho = 1 + rand() % 10;
        int *array = malloc(tamanho * sizeof(int));
        printf("Tamanho do array: %d\n",tamanho);
        printf("Numeros gerados: ");
        for(int i = 0; i < tamanho; i++) {
            array[i] =1 + rand() % x_recebido;
            printf("%d",array[i]);
            if(i<tamanho-1){
                printf(",");
            }else{
                printf("\n");
            }
        }

        // Passo 7: Enviar array para P2
        write(pipe_p1_p2[1], &tamanho, sizeof(int));
        write(pipe_p1_p2[1], array, tamanho * sizeof(int));
        close(pipe_p1_p2[1]);
        free(array);
        exit(EXIT_SUCCESS);
    } 
    else {
        // Processo P0 continua
        // Passo 2: Criar processo P2
        pid_t p2 = fork();
        if(p2 == 0) {
            // Processo P2
            close(pipe_p1_p2[1]);    // Fecha extremidade de escrita do pipe P1->P2
            close(pipe_p2_pai[0]);   // Fecha extremidade de leitura do pipe P2->P0

            close(pipe_pai_p1[0]);
            close(pipe_pai_p1[1]);
            close(pipe_p3_pai[0]);
            close(pipe_p3_pai[1]);

            // Passo 7: Receber array de P1
            int tamanho;
            read(pipe_p1_p2[0], &tamanho, sizeof(int));
            int *array = malloc(tamanho * sizeof(int));
            read(pipe_p1_p2[0], array, tamanho * sizeof(int));
            close(pipe_p1_p2[0]);
            // Passo 8: Calcular soma e enviar para P0
            int soma = 0;
            for(int i = 0; i < tamanho; i++) soma += array[i];
            
            write(pipe_p2_pai[1], &soma, sizeof(int));
            close(pipe_p2_pai[1]);
            free(array);
            exit(EXIT_SUCCESS);
        } 
        else {
            // Processo P0 continua
            // Passo 3 e 4: Enviar dados para P1
            close(pipe_pai_p1[0]);  // Fecha extremidade de leitura do pipe P0->P1
            close(pipe_p2_pai[1]); // Fecha extremidade de escrita do pipe p2->p0
            char *msg = "Meu filho, crie e envie para o seu irmão um array de números inteiros com valores randômicos entre 1 e o valor enviado anteriormente. O tamanho do array também deve ser randômico, na faixa de 1 a 10.";
            
            write(pipe_pai_p1[1], &X, sizeof(int));
            write(pipe_pai_p1[1], msg, strlen(msg)+1);
            close(pipe_pai_p1[1]);

            // Esperar P1 e P2 terminarem
            waitpid(p1, NULL, 0);
            waitpid(p2, NULL, 0);

            // Passo 9: Receber e mostrar soma de P2
            int soma;
            read(pipe_p2_pai[0], &soma, sizeof(int));
            close(pipe_p2_pai[0]);
            printf("Soma recebida por P0: %d\n", soma);

            // Passo 10: Criar P3 para executar 'date'
            pid_t p3 = fork();
            if(p3 == 0) {
                // Processo P3
                dup2(pipe_p3_pai[1], STDOUT_FILENO);  // Redireciona saída
                close(pipe_p3_pai[0]);
                close(pipe_p3_pai[1]);
                execlp("date", "date", NULL);
                perror("Erro no exec");
                exit(EXIT_FAILURE);
            } 
            else {
                // Processo P0 recebe saída de P3
                close(pipe_p3_pai[1]);
                char data[BUFFER_SIZE];
                read(pipe_p3_pai[0], data, BUFFER_SIZE);
                close(pipe_p3_pai[0]);
                printf("Data recebida de P3: %s", data);
                waitpid(p3, NULL, 0);
            }
        }
    }
    return 0;
}
