typedef struct decoded_message
{
    char key[256];
    char value[256];
    int status_code;
}decoded_message;

char* encode(char *status_code,char key[256],char val[256]);
void decode(char message[513],decoded_message *dec_mess);
