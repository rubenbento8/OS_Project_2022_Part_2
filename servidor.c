/******************************************************************************
 ** ISCTE-IUL: Trabalho prático 2 de Sistemas Operativos
 **
 ** Aluno: Nº: 88047     Nome: Rúben Bento
 ** Nome do Módulo: servidor.c v1
 ** Descrição/Explicação do Módulo: 
 **
 **
 ******************************************************************************/
#include "common.h"
#include "utils.h"
// #define DEBUG_MODE FALSE                     // To disable debug messages, uncomment this line

/* Variáveis globais */
Passagem pedido;                                // Variável que tem o pedido enviado do Cliente para o Servidor
Passagem lista_passagens[NUM_PASSAGENS];        // BD com o Número de pedidos em simultâneo que o servidor suporta
int indice_lista;                               // Índice corrente da Lista, que foi reservado pela função reservaEntradaBD()
Contadores stats;                               // Contadores de estatisticas

/* Protótipos de funções */
int init(Passagem*);                            // S1:   Função a ser implementada pelos alunos
int loadStats(Contadores*);                     // S2:   Função a ser implementada pelos alunos
int criaFicheiroServidor();                     // S3:   Função a ser implementada pelos alunos
int criaFifo();                                 // S4:   Função a ser implementada pelos alunos
int armaSinais();                               // S5:   Função a ser implementada pelos alunos
Passagem lePedido();                            // S6:   Função a ser implementada pelos alunos
int validaPedido(Passagem);                     // S7:   Função a ser implementada pelos alunos
int reservaEntradaBD(Passagem*, Passagem);      // S8:   Função a ser implementada pelos alunos
int apagaEntradaBD(Passagem*, int);             //       Função a ser implementada pelos alunos
int criaServidorDedicado(Passagem*, int);       // S9:   Função a ser implementada pelos alunos
void trataSinalSIGINT(int);                     // S10:  Função a ser implementada pelos alunos
void trataSinalSIGHUP(int, siginfo_t*, void*);  // S11:  Função a ser implementada pelos alunos
void trataSinalSIGCHLD(int);                    // S12:  Função a ser implementada pelos alunos
int sd_armaSinais();                            // SD13: Função a ser implementada pelos alunos
int sd_iniciaProcessamento(Passagem);           // SD14: Função a ser implementada pelos alunos
int sd_sleepRandomTime();                       // SD15: Função a ser implementada pelos alunos
int sd_terminaProcessamento(Passagem);          // SD16: Função a ser implementada pelos alunos
void sd_trataSinalSIGTERM(int);                 // SD17: Função a ser implementada pelos alunos
void ignora(int);                               // Função para Ignorar


int main() {    // Não é suposto que os alunos alterem nada na função main()
    // S1
    exit_on_error(init(lista_passagens), "Init");
    // S2
    exit_on_error(loadStats(&stats), "loadStats");
    // S3
    exit_on_error(criaFicheiroServidor(), "criaFicheiroServidor");
    // S4
    exit_on_error(criaFifo(), "criaFifo");
    // S5
    exit_on_error(armaSinais(), "armaSinais");

    while (TRUE) {  // O processamento do Servidor é cíclico e iterativo
        // S6
        pedido = lePedido();
        if (pedido.tipo_passagem < 0)
            continue;
        // S7
        if (validaPedido(pedido) < 0)
            continue;
        // S8
        indice_lista = reservaEntradaBD(lista_passagens, pedido);
        if (indice_lista < 0)
            continue;
        // S9
        int pidFilho = criaServidorDedicado(lista_passagens, indice_lista);
        if (pidFilho < 0) {
            apagaEntradaBD(lista_passagens, indice_lista);
            continue;
        } else if (pidFilho > 0) // Processo Servidor - Pai
            continue;

        // Processo Servidor Dedicado - Filho
        // SD13
        exit_on_error(sd_armaSinais(), "sd_armaSinais");
        // SD14
        exit_on_error(sd_iniciaProcessamento(pedido), "sd_iniciaProcessamento");
        // SD15
        exit_on_error(sd_sleepRandomTime(), "sd_sleepRandomTime");
        // SD16
        exit_on_error(sd_terminaProcessamento(pedido), "sd_terminaProcessamento");
    }
}

/**
 *  O módulo Servidor de Passagens é responsável pelo processamento de pedidos de passagem que chegam ao sistema  Scut-IUL.
 *  Este módulo é, normalmente, o primeiro dos dois (Cliente e Servidor) a ser executado, e deverá estar sempre ativo,
 *  à espera de pedidos de passagem. O tempo de processamento destes pedidos varia entre os MIN_PROCESSAMENTO segundos
 *  e os MAX_PROCESSAMENTO segundos. Findo esse tempo, este módulo sinaliza ao condutor de que a sua passagem foi processada.
 *  Este módulo deverá possuir contadores de passagens por tipo, um contador de anomalias e uma lista com capacidade para processar NUM_PASSAGENS passagens.
 *  O módulo Servidor de Passagens é responsável por realizar as seguintes tarefas:
 */

/**
 * S1   Inicia a lista de passagens, preenchendo em todos os elementos o campo tipo_passagem=-1 (“Limpa” a lista de passagens).
 *      Deverá manter um contador por cada tipo de passagem e um contador para as passagens com anomalia,
 *      devendo todos os contadores ser iniciados a 0. Em seguida, dá success S1 "Init Servidor";
 *
 * @return int Sucesso
 */
int init(Passagem* bd) {

    for(int i = 0; i < NUM_PASSAGENS; i++){         //corre a lista toda e limpa a lista pondo em todos os campos o tipo_passagem a -1
        bd[i].tipo_passagem = -1;
    }
    success("S1", "Init Servidor");
    return 0;
}

/**
 * S2   Se o ficheiro estatisticas.dat existir na diretoria local, abre esse ficheiro e lê os seus dados (em formato binário, ver formato em S10.4)
 *      para carregar o valor guardado de todos os contadores. Se houver erro em qualquer das operações, dá error S2 "<Problema>", caso contrário,
 *      dá success S2 "Estatísticas Carregadas";
 *
 * @return int Sucesso
 */
int loadStats(Contadores* pStats) {

    FILE *fic; 

    if (access(FILE_STATS, F_OK) != 0){                 //verifica se o ficheiro estatisticas.dat existe, se não existir inicia todos os contadores a 0
        pStats->contadorNormal = 0;                     //caso exista continua
        pStats->contadorViaVerde = 0;
        pStats->contadorAnomalias = 0;
        success("S2", "Estatísticas Iniciadas");
        return (0);
    }

    if((fic = fopen(FILE_STATS, "r")) == NULL){         //verifica se consegue abrir o ficheiro estatisticas.dat, caso contrário dá erro e encerra
        error("S2", "Erro de Leitura");
        return(-1);
    }
    fread( pStats, sizeof(Contadores), 1, fic );        //após verificar que existe e consegue abrir, lê o ficheiro em binário e carrega os valores para os contadores
    fclose(fic);

    success("S2", "Estatísticas Carregadas");
    return 0;
}

/**
 * S3   Cria o ficheiro FILE_SERVIDOR, e escreve nesse ficheiro o PID do Servidor (em formato de texto).
 *      Se houver erro em qualquer das operações, dá error S3, caso contrário, dá success S3 "<PID Servidor>";
 *
 * @return int Sucesso
 */
int criaFicheiroServidor() {

    FILE *pids;

    if((pids = fopen(FILE_SERVIDOR, "w")) != NULL){             //verifica se consegue criar o ficheiro em Write, se conseguir escreve o seu pid para o ficheiro
        fprintf(pids, "%d", getpid());
        fclose(pids);
    }
    else{
        error("S3", "Problemas ao criar Ficheiro do Servidor"); //Caso não consiga criar dá erro e encerra
        return(-1); 
    }

    success("S3", "%d", getpid());
    return 0;
}

/**
 * S4   Cria o ficheiro com organização FIFO (named pipe) FILE_PEDIDOS. 
 *      Se houver erro na operação, dá error S4 "<Problema>", caso contrário, dá  success S4 "Criei FIFO";
 *
 * @return int Sucesso
 */
int criaFifo() {

    if (mkfifo(FILE_PEDIDOS, 0666) == 0) {                  //cria um ficheiro do tipo FIFO com as permissões em 666, e verifica se consegue criar, caso consiga dá sucesso
        success("S4", "Criei FIFO");
        return 0;
    } 
    else {
        error("S4", "Problemas ao criar FIFO");             //se não conseguir dá erro e encerra
        return(-1); 
    }

    return 0;
}

/**
 * S5   Arma e trata os sinais SIGINT (ver S10), SIGHUP (ver S11) e SIGCHLD (ver S12).
 *      Depois de armar os sinais, dá success S5 "Armei sinais";
 *
 * @return int Sucesso
 */
int armaSinais() {

    struct sigaction action = {                 //sigaction e a sua estrutura
        .sa_flags = SA_SIGINFO,
        .sa_sigaction = trataSinalSIGHUP        //chama a função que será usada quando recebe o sinal
    };

	if (sigaction( SIGHUP, &action, NULL) < 0 )         //verifica se dá erro ao armar o sigaction, caso dê erro encerra
	{ 
        error("S5", "Erro no sigaction");               
        return -1;
	}

    signal(SIGINT, trataSinalSIGINT);               //armas os dois sinais, SIGINT e SIGCHLD e dá success
    signal(SIGCHLD, trataSinalSIGCHLD);

    success("S5", "Armei sinais");
    return 0;
}

/**
 * S6   Lê a informação do FIFO FILE_PEDIDOS, (em formato binário) que deverá ser um elemento do tipo Passagem.
 *      Se houver erro na operação, dá error S6 "<Problema>", caso contrário, dá success S6 "Li FIFO";
 *
 * @return Passagem Elemento com os dados preenchidos. Se tipo_passagem = -1, significa que o elemento é inválido
 */
Passagem lePedido() {
    FILE *fic; 
    Passagem p;
    p.tipo_passagem = -1;   // Por omissão, retorna valor inválido

    if((fic = fopen(FILE_PEDIDOS, "r")) == NULL){                   //verifica se consegue ler o ficheiro pedidos.fifo, caso não consiga dá erro e encerra
        error("S6", "Problema a ler ficheiro %s", FILE_PEDIDOS);
        return(p);
    }
    if (fread( &p, sizeof(p), 1, fic ) < 1){               //caso consiga lê a primeira entrada em binário, e verifica se o mesmo teve problemas na leitura, caso tenha dá erro e encerra
        error("S6", "Problemas na leitura");
        p.tipo_passagem = -1;
        fclose(fic);
        return p;
    }

    success("S6", "Li FIFO");                     //caso consiga ler, guarda a leitura na variável p, dá success e fecha o ficheiro

    fclose(fic);
    return p;
}

/**
 * S7   O Servidor deve validar se o pedido está corretamente formatado. A formatação correta de um pedido será:
 *      •  O Tipo de passagem é válido (1 para pedido Normal, ou 2 para Via Verde);
 *      •  A Matrícula e o Lanço não são strings vazias (não é necessário fazer mais validações sobre o seu conteúdo);
 *      •  O pid_cliente é um valor > 0. Não se fazem validações sobre pid_servidor_dedicado.
 *      Em caso de erro na formatação do pedido, dá error S7, indicando qual foi o erro detetado, 
 *      incrementa o contador de anomalias, ignora o pedido, e recomeça o processo no passo S6. Caso contrário, 
 *      dá success S7 "Chegou novo pedido de passagem do tipo <Normal | Via Verde> solicitado pela viatura com matrícula <matricula> para o Lanço <lanco> e com PID <pid_cliente>";
 *
 * @return int Sucesso
 */
int validaPedido(Passagem pedido) {
    int erro = FALSE;               //cria 2 booleans de suporte
    int erroPID = FALSE;
    char tipo[10] = " ";            //cria uma string que será usada mais tarde

    if(pedido.tipo_passagem == 1){              //verifica qual o valor da passagem, caso seja 1, para passagem Normal ou 2 para passagem Via Verde 
        strcpy(tipo, "Normal");                 //caso seja 1, escreve na string "Normal"
    }
    else{
        if(pedido.tipo_passagem == 2){          //caso seja 2, escreve na string "Via Verde"
           strcpy(tipo, "Via Verde");
        }
        else{
            erro = TRUE;                                //caso não seja nenhum dos dois, o boolean erro passa a TRUE e informa o erro
            error("S7", "Tipo de Passagem Inválida");
        }
    }

    if(!strcmp(pedido.matricula, "")){                  //verifica se a matricula do pedido é null, caso seja o boolean erro passa a TRUE e informa o erro
        erro = TRUE;
        error("S7", "Matricula Inválida");
    }
    if(!strcmp(pedido.lanco, "")){                      //verifica se o lanço do pedido é null, caso seja o boolean erro passa a TRUE e informa o erro
        erro = TRUE;
        error("S7", "Lanço Inválido");
    }
    if(pedido.pid_cliente <= 0){                        //verifica se o pid_cliente do pedido é maior que 0, caso não seja o boolean erro e erroPID passa a TRUE e informa o erro
        erro = TRUE;
        erroPID = TRUE;
        error("S7", "Pid Cliente Inválido");
    }
    if(erro && !erroPID){                //verifica se deu erro e que o erro não é no pid_cliente, caso haja erro mas não no pid_cliente envia um sinal do tipo SIGHUP
        kill(pedido.pid_cliente, SIGHUP);   
    }

    if(erro){                           //verifica se existe algum erro, caso haja aumenta o contador de anomalias em 1 e dá return de -1
        stats.contadorAnomalias++;
        return (-1);
    }

    success("S7", "Chegou novo pedido de passagem do tipo %s solicitado pela viatura com matrícula %s para o Lanço %s e com PID %d", tipo, pedido.matricula, pedido.lanco, pedido.pid_cliente);
    return 0;           //caso não haja erro, passa a mensagem de sucesso usando a variável string criada para informar qual foi o tipo de passagem e dá return de 1
}

/**
 * S8   Verifica se existe disponibilidade na Lista de Passagens. Se todas as entradas da Lista de Passagens estiverem ocupadas, 
 *      dá error S8 "Lista de Passagens cheia", incrementa o contador de passagens com anomalia, manda o sinal SIGHUP ao processo 
 *      com PID <pid_cliente>, ignora o pedido, e recomeça o processo no passo S6. 
 *      Caso contrário, preenche uma entrada da lista com os dados deste pedido, incrementa o contador de passagens do tipo de passagem correspondente 
 *      e dá success S8 "Entrada <índice lista> preenchida";
 *
 * @return int Em caso de sucesso, retorna o índice da lista preenchido. Caso contrário retorna -1
 */
int reservaEntradaBD(Passagem* bd, Passagem pedido) {
    int indiceLista = -1;           //inicia o indice a -1 para controlo de erros
    for(int i = 0; i < NUM_PASSAGENS; i++){     //corre a lista de passagens, mal encontre um caso em que o tipo_passagem seja -1 para o loop e guarda a variável do indice
        if(bd[i].tipo_passagem == -1){          //e guarda a passagem na posição do indice
            indiceLista = i;
            bd[i] = pedido;
            break;
        }
    }
    if (indiceLista == -1){                 //verifica se o indice é -1, significando que o mesmo não encontrou nenhum lugar na lista vazio e informa o cliente enviando um sinal
        error("S8", "Lista de Passagens cheia");    //do tipo SIGHUP para ele e aumenta o contador de anomalias em 1
        kill(pedido.pid_cliente, SIGHUP);
        stats.contadorAnomalias++;
    }
    else{
        if(pedido.tipo_passagem == 1){      //verifica qual o tipo de passagem se for 1 aumenta o contadorNormal em 1, caso seja 2 aumenta o contadorViaVerde em 1
            stats.contadorNormal++;         //sendo que é validado no S7 o tipo de passagem o mesmo aqui só poderá ser 1 ou 2
        }
        else{
            stats.contadorViaVerde++;
        }
        success("S8", "Entrada %d preenchida", indiceLista); //dá success, informa que foi preenchida qual a entrada que foi preenchida e devolve o indice
    }
    return indiceLista;
}

/**
 * "Apaga" uma entrada da Lista de Passagens, colocando tipo_pedido = -1
 *
 * @return int Sucesso
 */
int apagaEntradaBD(Passagem* bd, int indiceLista) {
    bd[indiceLista].tipo_passagem = -1;             //na lista de passagens, verifica a entrada com o indice dado e passa o tipo de passagem para -1
    return 0;
}

/**
 * S9   Cria um processo filho (fork) Servidor Dedicado. Se houver erro, dá error S9 "Fork". 
 *      Caso contrário: O processo Servidor Dedicado (filho) continua no passo SD13, 
 *      e o processo Servidor (pai) completa o preenchimento da entrada atual da Lista de Passagens com o PID do Servidor Dedicado, 
 *      e dá success S9 "Criado Servidor Dedicado com PID <pid Filho>". Em qualquer dos casos, de erro ou de sucesso, recomeça o processo no passo S6;
 *
 * @return int PID do processo filho, se for o processo Servidor (pai), 0 se for o processo Servidor Dedicado (filho), ou -1 em caso de erro.
 */
int criaServidorDedicado(Passagem* bd, int indiceLista) {
    int pidFilho = -1;          //inicia o pid do filho a -1 para controlo de erros
    pidFilho = fork();          //cria uma processo filho via fork
    if (pidFilho < 0) {         //verifica se o processo filho é criado com sucesso, caso contrário dá erro
        error("S9", "Fork");
    } 
    else {
        if (pidFilho > 0){      //caso crie com sucesso, verifica se o processo é o pai, caso seja dá success e mostra o pid do filho, depois altera na lista de passagens na entrada
            success("S9", "Criado Servidor Dedicado com PID %d", pidFilho);   //do indice o pid do servidor dedicado pelo o pid do filho
            bd[indiceLista].pid_servidor_dedicado = pidFilho;
        }
    } 
            
    return pidFilho;
}

/**
 * S10  O sinal armado SIGINT serve para o Diretor da Portagem encerrar o Servidor, usando o atalho <CTRL+C>. 
 *      Se receber esse sinal (do utilizador via Shell), o Servidor dá success S10 "Shutdown Servidor", e depois:
 *      S10.1   Envia o sinal SIGTERM a todos os Servidores Dedicados da Lista de Passagens, 
 *              para que concluam o seu processamento imediatamente. Depois, dá success S10.1 "Shutdown Servidores Dedicados";
 *      S10.2   Remove o ficheiro servidor.pid. Em caso de erro, dá error S10.2, caso contrário, dá success S10.2;
 *      S10.3   Remove o FIFO pedidos.fifo. Em caso de erro, dá error S10.3, caso contrário, dá success S10.3;
 *      S10.4   Cria o ficheiro estatisticas.dat, escrevendo nele o valor de 3 inteiros (em formato binário), correspondentes a
 *              <contador de passagens Normal>  <contador de passagens Via Verde>  <contador Passagens com Anomalia>
 *              Em caso de erro, dá error S10.4, caso contrário, dá success S10.4 "Estatísticas Guardadas";
 *      S10.5   Dá success S10.5 e termina o processo Servidor.
 */
void trataSinalSIGINT(int sinalRecebido) {
    int shut = FALSE;                               //cria uma variável do tipo boolean
    success("S10", "Shutdown Servidor");
    for(int i = 0; i < NUM_PASSAGENS; i++){          //corre a lista de passagens toda, qualquer entrada que tenha o tipo de passagem diferente de -1 será mandado um sinal
        if(lista_passagens[i].tipo_passagem != -1){     //do tipo sigterm para o servidor dedicado da e passa a variável boolean para TRUE
            shut = TRUE;
            kill(lista_passagens[i].pid_servidor_dedicado, SIGTERM);
        }
    }
    if(shut){       //verifica se deu shutdown a algum servidor, caso tenha dá success como informação
        success("S10.1", "Shutdown Servidores Dedicados");
    }
    if (remove(FILE_SERVIDOR) == 0){        //tenta apaghar o ficheiro servidor.pid, caso consiga dá success caso contrário dá erro
        success("S10.2", "");
    }
    else{
        error("S10.2", "");
    }

    if (remove(FILE_PEDIDOS) == 0){         //tenta apagar o ficheiro pedidos.fifo, caso consiga dá success caso contrário dá erro
        success("S10.3", "");
    }
    else{
        error("S10.3", "");
    }

    FILE * fb = fopen( FILE_STATS, "w" );   //tenta criar o ficheiro estatisticas.dat, caso consiga guarda a variável global stats em binário no ficheiro e dá success
        if (fb == NULL){                    //caso contrário dá erro
            error("S10.4", "");
        }
        else{
            fwrite(&stats, sizeof(stats), 1, fb);
            success("S10.4", "Estatísticas Guardadas"); 
            fclose(fb);
        }
    success("S10.5", "");       
    exit (-1);                  //no final encerra o servidor
}

/**
 * S11  O sinal armado SIGHUP serve para o Cliente indicar que deseja cancelar o pedido de processamento a passagem. 
 *      Se o Servidor receber esse sinal, dá success S11 "Cancel", e em seguida, terá de fazer as seguintes ações:
 *      S11.1   Identifica o PID do processo Cliente que enviou o sinal (usando sigaction), dá success S11.1 "Cancelamento enviado pelo Processo <PID Cliente>";
 *      S11.2   Pesquisa na Lista de Passagens pela entrada correspondente ao PID do Cliente que cancelou. Se não encontrar, dá error S11.2.
 *              Caso contrário, descobre o PID do Servidor Dedicado correspondente, dá success S11.2 "Cancelamento <PID Filho>";
 *      S11.3   Envia o sinal SIGTERM ao Servidor Dedicado da Lista de Passagens correspondente ao cancelamento, 
 *              para que conclua o seu processamento imediatamente. Depois, dá success S10.1 "Cancelamento Shutdown Servidor Dedicado", 
 *              e recomeça o processo no passo S6.
 */
void trataSinalSIGHUP(int sinalRecebido, siginfo_t *sig_info, void *x){

    success("S11", "Cancel");

    pid_t pid_proc = sig_info -> si_pid;      //descobre o pid do cliente e guarda em uma variável

    success("S11.1", "Cancelamento enviado pelo Processo %d", pid_proc);    

    int indice = -1;    //inicia a varíavel indice a -1 para controlo de erros

    for(int i = 0; i < NUM_PASSAGENS; i++){
        if(lista_passagens[i].pid_cliente == pid_proc && lista_passagens[i].tipo_passagem != -1){ //procura na lista de passagens uma entrada com o pid_cliente e tipo de passagem diferente de -1
            indice = i;         //caso encontre guarda o indice i na varíavel indice e pára o loop
            break;
        }
    }
    if(indice == -1){       //caso a varíavel indice seja -1, não apagou nenhuma entrada dá erro e dá return
        error("S11.2", "Passagem não encontrada");
        return;
    }
    else{       //caso tenha apagado, dá success, aumenta o contador de anomalias em mais 1 e limpa a entrada da lista pondo o tipo de passagem a -1
        success("S11.2", "Cancelamento %d", lista_passagens[indice].pid_servidor_dedicado);
        lista_passagens[indice].tipo_passagem = -1;
        stats.contadorAnomalias++;
    }
    kill(lista_passagens[indice].pid_servidor_dedicado, SIGTERM);       //no final ainda passa ao servidor dedicado responsável pela passagem o sinal SIGTERM
    success("S11.3", "Sinal de Cancelamento enviado ao Servidor Dedicado");
    return;
}

/**
 * S12  O sinal armado SIGCHLD serve para que o Servidor seja alertado quando um dos seus filhos Servidor Dedicado terminar.
 *      Se o Servidor receber esse sinal, dá success S12 "Servidor Dedicado Terminou", e em seguida, terá de fazer as seguintes ações:
 *      S12.1   Identifica o PID do Servidor Dedicado que terminou (usando wait), dá success S12.1 "Terminou Servidor Dedicado <PID Filho>";
 *      S12.2   Pesquisa na Lista de Passagens pela entrada correspondente ao PID do Filho que terminou. 
 *              Se não encontrar, dá error S12.2. Caso contrário, “apaga” a entrada da Lista de Passagens 
 *              correspondente (colocando tipo_passagem=-1), dá success S12.2, e recomeça o processo no passo S6.
 */
void trataSinalSIGCHLD(int sinalRecebido) {
    success("S12", "Servidor Dedicado Terminou");
    int cpid = wait(NULL);      //usando o wait descobre o PID do filho que morreu
    int apagou = FALSE;         //cria uma variável boolean
    success("S12.1", "Terminou Servidor Dedicado <%d>", cpid);

    for(int i = 0; i < NUM_PASSAGENS; i++){     //percorre a lista de passagens inteira 
        if(lista_passagens[i].pid_servidor_dedicado == cpid && lista_passagens[i].tipo_passagem != -1){    //caso encontre uma entrada que contenha o pid do filho em pid_servidor_dedicado
            lista_passagens[i].tipo_passagem = -1;      //e cujo tipo de passagem também não seja -1, passa o tipo de passagem para -1, passa a variável boolean para TRUE e pára o loop
            apagou = TRUE;
            break;
        }
    }
    if(apagou){                 //verifica se foi apagada a entrada da lista, caso tenha sido dá success, caso não tenha sido dá error
        success("S12.2", "");
    }
    else{
        error("S12.2", "");
    }
    return;
}

/**
 * SD13 O novo processo Servidor Dedicado (filho) arma os sinais SIGTERM (ver SD17) e SIGINT (programa para ignorar este sinal). 
 *      Depois de armar os sinais, dá success SD13 "Servidor Dedicado Armei sinais";
 *
 * @return int Sucesso
 */
int sd_armaSinais() {
    signal(SIGTERM, sd_trataSinalSIGTERM);      //arma o sinal SIGTERM
    signal(SIGINT, SIG_IGN);                    //arma o sinal SIGINT para o ignorar
    success("SD13", "Servidor Dedicado <%d> Armei sinais", getpid());
    return 0;
}

/**
 * SD14 O Servidor Dedicado envia o sinal SIGUSR1, indicando o início do processamento da passagem, ao processo <pid_cliente> 
 *      que pode obter da estrutura Passagem do pedido que “herdou” do Servidor ou da entrada da Lista de Passagens, 
 *      e dá success SD14 "Início Passagem <PID Cliente> <PID Servidor Dedic>";
 *
 * @return int Sucesso
 */
int sd_iniciaProcessamento(Passagem pedido) {
    kill(pedido.pid_cliente, SIGUSR1);      //envia um sinal do tipo SIGUSR1 para o pid_cliente presente na passagem pedido
    success("SD14", "Início Passagem de <%d> por <%d>", pedido.pid_cliente , getpid());
    return 0;
}

/**
 * SD15 O Servidor Dedicado calcula um valor de tempo aleatório entre os valores MIN_PROCESSAMENTO e MAX_PROCESSAMENTO, 
 *      dá success SD15 "<Tempo>", e aguarda esse valor em segundos (sleep);
 *
 * @return int Sucesso
 */
int sd_sleepRandomTime() {
    int tempo = my_rand()%(MAX_PROCESSAMENTO-MIN_PROCESSAMENTO + 1) + MIN_PROCESSAMENTO;   //calcula um valor aleatório entre MAX_PROCESSAMENTO e MIN_PROCESSAMENTO
    success("SD15", "Tempo: %d", tempo);    //depois o servidor dedicado adormece durante esse tempo aleatório em segundos
    sleep(tempo);
    return 0;
}

/**
 * SD16 O Servidor Dedicado envia o sinal SIGTERM, indicando o fim do processamento da passagem, ao processo <pid_cliente>, 
 *      dá success SD16 "Fim Passagem <PID Cliente> <PID Servidor Dedicado>", e termina o Servidor Dedicado;
 *
 * @return int Sucesso
 */
int sd_terminaProcessamento(Passagem pedido) {
    kill(pedido.pid_cliente, SIGTERM);      //envia um sinal do tipo SIGTERM para o pid_cliente presente na passagem pedido
    success("SD16", "Fim Passagem de %d por %d", pedido.pid_cliente, getpid());
    exit (-1);
    return 0;
}

/**
 * SD17 O sinal armado SIGTERM serve para o Servidor indicar que deseja terminar imediatamente o pedido de processamento da passagem.
 *      Se o Servidor Dedicado receber esse sinal, envia o sinal SIGHUP ao <pid_cliente>, 
 *      dá success SD17 "Processamento Cancelado", e termina o Servidor Dedicado.
 */
void sd_trataSinalSIGTERM(int sinalRecebido) {
    kill(lista_passagens[indice_lista].pid_cliente, SIGHUP);  //envia o sinal SIGHUP para o pid cliente presente na passagem na lista de passagens na entrada com a varíavel global indice_lista
    success("SD17", "Processamento Cancelado");
    exit (-1);
}