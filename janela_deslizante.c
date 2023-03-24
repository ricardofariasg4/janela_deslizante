#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>

#define TAMANHO_JANELA 2 //2 pacotes por vez, 200 bytes
#define TAMANHO_ARQUIVO 1024 //Um arquivo hipotético de 1 KB
#define TAMANHO_DADO 92 //Tamanho do dado dentro do pacote
#define PAUSE getchar();

//Um pacote hipotético de 97 bytes 
typedef struct _PACOTE_ARQUIVO { 
    char dado[92]; //92 bytes
    int checksum; //4 bytes
    unsigned char index; //1 byte
} PACOTE_ARQUIVO;

void limpaBuffer (void)
{
    char c;
    while ((c = getchar()) != '\n' && c != EOF);
}

int gerar_checksum (PACOTE_ARQUIVO *pacote) 
{
    int i, checksum=0;
    
    for (i=0; i<sizeof(pacote->dado); i++)
        checksum += pacote->dado[i];
    
    return checksum;
}

//Simula um meio de envio (como um cabo ethernet por exemplo)
void envia_pacote (PACOTE_ARQUIVO *pacote)
{
    char decisao;
    printf("\n\tPacote a ser enviado: %s", pacote->dado);
    
    //Uma simulação de erro
    printf("\n\tInserir erro no pacote? [s/n] ");
    scanf("%c", &decisao);
    limpaBuffer();

    if (decisao=='s' || decisao=='S')
    {
        for (int i=10; i<20; i++)
            pacote->dado[i] = '*';
    }
}

//Se retornar 1 é ACK, se retornar 0 é NACK
int recebe_pacote (PACOTE_ARQUIVO *pacote, FILE **destino, unsigned char *buffer)
{
    int meu_checksum = gerar_checksum(pacote);

    printf("\n\tbuffer = %d && index = %d", *buffer, pacote->index);
    printf("\n\tchecksum %s && seq %s", meu_checksum==pacote->checksum ? "ok" : "not ok", pacote->index==*buffer ? "correta" : "incorreta");

    if (meu_checksum==pacote->checksum && pacote->index==*buffer)
    {
        if (*buffer<256) 
            *buffer = *buffer + 1;
        else 
            *buffer = 0;
        
        //Apenas para fins de simulação estou controlando diretamente os valores
        if (pacote->index == 11)
            fwrite(&pacote->dado, sizeof(char), 23, *destino);
        else
            fwrite(&pacote->dado, sizeof(char), sizeof(pacote->dado)-1, *destino);

        return 1;
    }

    return 0;
}

int main (void)
{
    //Estrutura do emissor
    FILE *origem = fopen("origem.txt", "r");

    //Estruturas do receptor
    FILE *destino = fopen("destino.txt", "w");
    unsigned char buffer = 0;

    //Estruturas da simulacao (o programa)
    int 
        i=0, 
        j=0,
        k=0,
        base = 0,  
        voltan = -1, 
        qtd_pacotes = 0, 
        restante = TAMANHO_ARQUIVO;

    PACOTE_ARQUIVO pacote;
    int ack[TAMANHO_JANELA];
    
    if (!origem || !destino) {
        perror("fopen");
        exit(1);
    }

    //10 pacotes inteiros para enviar
    qtd_pacotes = ( TAMANHO_ARQUIVO / (TAMANHO_DADO-1) ); 
    if ( (TAMANHO_ARQUIVO % (TAMANHO_DADO-1)) > 0 ) qtd_pacotes+=1;
    
    printf("\nPacotes a enviar = %d", qtd_pacotes);
    
    while (base < qtd_pacotes)
    {
        printf("\nJanela %d ->", (base+1));
        
        //Para o caso de restar menos de 1 pacote para envio
        if (restante < TAMANHO_DADO)
        {
            printf("\nPacote restante (%d até %d)", j, restante); 
            PAUSE

            //Preparando o pacote
            fread(pacote.dado, sizeof(char), restante, origem); 
            pacote.dado[restante] = '\0';
            pacote.checksum = gerar_checksum(&pacote);
            pacote.index = base;

            if (pacote.index > 255)
                pacote.index = base % 256;
            
            //Ação sob o pacote
            envia_pacote(&pacote); //Simula meio de envio
            printf("\tPacote a ser enviado: %s ", pacote.dado);
            ack[0] = recebe_pacote(&pacote, &destino, &buffer); //Simula meio de recepção            

            //Vou verificar o pacote logo de cara pois é apenas 1 pacote pequeno
            while (!ack[0]) 
            {
                fseek(origem, -restante, SEEK_CUR);
                fread(pacote.dado, sizeof(char), restante, origem); 
                pacote.dado[restante] = '\0';
                pacote.checksum = gerar_checksum(&pacote);
                envia_pacote(&pacote);
                ack[0] = recebe_pacote(&pacote, &destino, &buffer);    
            }

            base+=qtd_pacotes;
            restante = TAMANHO_DADO;
        }

        else
        {
            //Estou basicamente enviando a janela inteira sem conferir
            while (i < TAMANHO_JANELA)
            {
                printf("\n\tPacote %d: bytes[%d -> %d]", i, j+1, j+(TAMANHO_DADO-1));
                
                //Preparando o pacote
                fread(pacote.dado, sizeof(char), TAMANHO_DADO-1, origem); 
                pacote.dado[TAMANHO_DADO-1] = '\0';
                pacote.checksum = gerar_checksum(&pacote);
                pacote.index = base; //Sequencializacao

                //Enumerando os pacotes sempre de 0 a 255
                if (pacote.index > 255)
                    pacote.index = base % 256;

                //Ação sob o pacote
                envia_pacote(&pacote); //Simula meio de envio
                printf("\tPacote a ser enviado: %s ", pacote.dado);
                ack[i] = recebe_pacote(&pacote, &destino, &buffer); //Simula meio de recepcao

                j+=(TAMANHO_DADO-1);
                restante = TAMANHO_ARQUIVO-j;
                i++;
                base++;

                if (restante < TAMANHO_DADO) break;
            }

            //Conferindo se houve algum erro de transmissão na janela
            voltan = -1;
            for (k=0; k<TAMANHO_JANELA; k++)
                if (!ack[k])
                    break;

            //Se k chegou a valer 2, então houve erro.
            if (k<2) 
                voltan = (TAMANHO_JANELA-1)-k+1; 
                
            //Se volta n é diferente de -1 (valor padrão) então houve problemas na transmissão dos pacotes
            if (voltan != -1)
            {
                i -= voltan;
                fseek(origem, -(voltan*(TAMANHO_DADO-1)), SEEK_CUR);
                j = j - (voltan*(TAMANHO_DADO-1));
                base = base - voltan; //Se eu tenho que voltar 2 casas é pq 2 pacotes tbm não foram enviados    
            }
            else
                i=0;

            PAUSE            
        }
    }

    fputc('\0', destino);
    fputc('\n', destino);
    
    fclose(origem);
    fclose(destino);
    
    return 0;
}