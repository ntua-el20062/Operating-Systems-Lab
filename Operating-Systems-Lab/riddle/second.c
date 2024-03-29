#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <fcntl.h>

int main() {
    
    //prepei na dhmiourgisoume ta secret_number file prin ginei to fork, wstena to klhronomhsei to paidi kai na paei na to gemisei  me ton secret number gia na to dei meta o mpampas kai na to ektypwsei
    //xreiazetai to file ama hsh yparxei apla na diagrafontai ta periexomena ara flag O_TRUNC, kai read, write permissions
    int fd = open("secret_number", O_CREAT | O_RDWR | O_TRUNC, S_IRUSR | S_IWUSR);
    
    pid_t childPid = fork();

    if (childPid == -1) {
        fprintf(stderr, "Fork failed\n");
        exit(1);
    }

    if (childPid == 0) {
         // paidi
         char path[]="./riddle";
         char *argv[]={path, NULL};
         char *envp[]={NULL};
         execve(path, argv, envp);
    }
    // mpampas
    
    sleep(4);
    
    //se ayto to shmeio o mpampas perimenei me to sleep to paidi na teleiwsei(ousiastika na ftasei to paidi sto shmeio opoy sthn ektypwsh toy programmatos leei what number im thiking right now?), twra
    //twra prepei o mpampas na kanei access sto koino file secret_number kai na paei na diavasei to noumero pou yparxei mesa. Gia na to kanei ayto tha prepei na kanei read to secret_number kai na valei
    //ta periexomena se enan buffer wste meta na mporei na ton ektypwsei. omws an kaname fgets gia na paroume olo to periexomeno toy secret_number tha ektypwname kai alla pragmata kai oxi mono ton
    //arthmo, ara prepei ston buffer mas meta thn klhsh toy read na diabasoume apo ena sygkekrimeno shmeio kai meta gia na paroume mono to noymero.

    //epishs se ayto to shmeio kalo tha htan gia na mhn diavazoume axrhsta bytes, na pame kai na arxikopoihsoume to offset apo to opoio tha ksekinhsei h anagnwsh gia na anagnwrisoume swsta ton arithmo
    char buf[8];
    ssize_t count, wcnt;
    off_t offset = lseek(fd, 62, SEEK_SET); //ksekiname apo to 62 giati theloume na paroume mono ton arithmo, oxi olo to string
    //vazoume edw ston buffer to secret number
    count =read(fd, buf, 8);
    //ektypwnoume
    wcnt = write(1, buf, 8);
    printf("1/n");
    //twra prepei na ektypwsoume sthn eksodo,edw ousiastika o pateras apantaei sto paidi ton arithmo poy psaxoume
    
   
        close(fd);
        printf("2/n");
        if(wait(NULL)==-1) {
                 perror("wait");
                exit(1);
        }
printf("3/n");

        return 0;
}
