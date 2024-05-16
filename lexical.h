#include <stdio.h>
#include <stdlib.h>
#include <ctype.h>
#include <string.h>
#include <stdarg.h>

void err(const char *fmt,...)
{
    va_list va;
    va_start(va,fmt);
    fprintf(stderr,"error: ");
    vfprintf(stderr,fmt,va);
    fputc('\n',stderr);
    va_end(va);
    exit(-1);
}

#define SAFEALLOC(var,Type) if((var=(Type*)malloc(sizeof(Type)))==NULL) err("not enough memory") ;

enum{ID, END, CT_INT, CT_REAL, CT_CHAR, CT_STRING, ASSIGN, SEMICOLON, BREAK, CHAR, DOUBLE, ELSE,
     FOR, IF, INT, RETURN, STRUCT, VOID, WHILE, EQUALS, COMMA, LPAR, RPAR, LBRACKET,
     RBRACKET, LACC, RACC, ADD, SUB, MUL, DIV, DOT, AND, OR, NOT, NOTEQ, LESS, LESSEQ, GREATER, GREATEREQ}; // tokens codes

const char* enum_values[] = {"ID", "END", "CT_INT", "CT_REAL", "CT_CHAR", "CT_STRING", "ASSIGN", "SEMICOLON", "BREAK",
     "CHAR", "DOUBLE", "ELSE", "FOR", "IF", "INT", "RETURN", "STRUCT", "VOID", "WHILE", "EQUALS",
     "COMMA", "LPAR", "RPAR", "LBRACKET", "RBRACKET", "LACC", "RACC", "ADD", "SUB", "MUL", 
     "DIV", "DOT", "AND", "OR", "NOT", "NOTEQ", "LESS", "LESSEQ", "GREATER", "GREATEREQ"};

typedef struct _Token{
    int code; // code (name)
    union{
        char *text; // used for ID, CT_STRING (dynamically allocated)
        long int i; // used for CT_INT, CT_CHAR
        double r; // used for CT_REAL
    };
    int line; // the input file line
    struct _Token *next; // link to the next token
}Token;

Token *tokens=NULL, *lastToken=NULL;
char *input_text;
int line=0, current_index=0;

void tkerr(const Token *tk,const char *fmt,...)
{
    va_list va;
    va_start(va,fmt);
    fprintf(stderr,"error in line %d: ",tk->line);
    vfprintf(stderr,fmt,va);
    fputc('\n',stderr);
    va_end(va);
    exit(-1);
}

void print_token(Token *start)
{
    // if(start->code==2)
    //     printf("%s -> %d\n", enum_values[start->code], start->i);
    // else if(start->code==4)
    //     printf("%s -> '%c'\n", enum_values[start->code], start->i);
    // else if(start->code==0)
    //     printf("%s -> %s\n", enum_values[start->code], start->text);
    // else if(start->code==3)
    //     printf("%s -> %f\n", enum_values[start->code], start->r);
    // else if(start->code==5)
    //     printf("%s -> %s\n", enum_values[start->code], start->text);
    // else
    printf("%s \n", enum_values[start->code]);
}

void print_list()
{
    Token *start = tokens;
    while(start!=lastToken)
    {
        print_token(start);
        start=start->next;
    }
}

Token *addTk(int code)
{
    Token *tk;
    SAFEALLOC(tk,Token)
    tk->code=code;
    tk->line=line;
    tk->next=NULL;
    if(lastToken != NULL){
        lastToken->next=tk;
    }else{
        tokens=tk;
    }
    printf("%s ", enum_values[code]);
    lastToken=tk;
    return tk;
}

char *createString(int x, int y)
{
    char *text = calloc(50, sizeof(char));
    int i;
    for(i=x; i<y; ++i)
        text[i-x]=input_text[i];
    text[y]='\0';
    return text;
}

//TODO base 8 and 16 numbers
int createInt(char *s)
{
    int n=0;
    for(int i=0; s[i]; ++i)
        n=n*10+s[i]-'0';
    return n;
}

//TODO handle numbers with e/E in them
double createFloat(char *s)
{
    double result=0;
    int powerof10=1;
    result=s[0]-'0';
    for(int i=2; s[i]; ++i)
    {
        result=result*10+s[i]-'0';
        powerof10*=10;
    }
    result/=powerof10;
    return result;
}

int getNextToken()
{
    int state=0;
    char ch;
    int starting_index=0, ending_index=0;
    Token *tk;
    while(1)
    {
        ch = input_text[current_index];
        switch(state)
        {
            case 0:
                {

                    if(isalpha(ch) || ch=='_')
                    {
                        starting_index=current_index;
                        ending_index=current_index;
                        current_index++;
                        state = 1;
                    }
                    else if('1'<=ch && ch<='9')
                    {
                        starting_index=current_index;
                        ending_index=current_index;
                        current_index++;
                        state=3;
                    }
                    else if(ch=='0')
                    {
                        starting_index=current_index;
                        ending_index=current_index;
                        current_index++;
                        state = 5;
                    }
                    else if(ch==' '||ch=='\r'||ch=='\t')
                    {
                        current_index++;
                    }
                    else if(ch=='\n')
                    {
                        current_index++;
                        line++;
                    }
                    else if(ch==0)
                    {
                        addTk(END);
                        return END;
                    }
                    else if(ch=='\'')
                    {
                        starting_index=current_index;
                        ending_index=current_index;
                        current_index++;
                        state=14;
                    }
                    else if(ch=='"')
                    {
                        starting_index=current_index;
                        ending_index=current_index;
                        current_index++;
                        state=18;
                    }
                    else if(ch=='/')
                    {
                        current_index++;
                        state=22;
                    }
                    else if(ch=='=')
                    {
                        current_index++;
                        state=26;
                    }
                    else if(ch==',')
                    {
                        current_index++;
                        state=27;
                    }
                    else if(ch==';')
                    {
                        current_index++;
                        state=28;
                    }
                    else if(ch=='(')
                    {
                        current_index++;
                        state=29;
                    }
                    else if(ch==')')
                    {
                        current_index++;
                        state=30;
                    }
                    else if(ch=='[')
                    {
                        current_index++;
                        state=31;
                    }
                    else if(ch==']')
                    {
                        current_index++;
                        state=32;
                    }
                    else if(ch=='{')
                    {
                        current_index++;
                        state=33;
                    }
                    else if(ch=='}')
                    {
                        current_index++;
                        state=34;
                    }
                    else if(ch=='+')
                    {
                        current_index++;
                        state=36;
                    }
                    else if(ch=='-')
                    {
                        current_index++;
                        state=37;
                    }
                    else if(ch=='*')
                    {
                        current_index++;
                        state=38;
                    }
                    else if(ch=='.')
                    {
                        current_index++;
                        state=39;
                    }
                    else if(ch=='&')
                    {
                        current_index++;
                        state=40;
                    }
                    else if(ch=='|')
                    {
                        current_index++;
                        state=41;
                    }
                    else if(ch=='!')
                    {
                        current_index++;
                        state=42;
                    }
                    else if(ch=='<')
                    {
                        current_index++;
                        state=46;
                    }
                    else if(ch=='>')
                    {
                        current_index++;
                        state=48;
                    }
                    else
                    {
                        current_index++;
                    }
                }
                break;
            case 1:
                {
                    if(isalnum(ch) || ch=='_')
                    {
                        current_index++;
                        ending_index++;
                    }
                    else
                        state = 2;

                }
                break;
            case 2: //final case: ID
                {
                    char *content = createString(starting_index, ending_index+1);
                    if(strcmp(content, "break")==0)
                        tk=addTk(BREAK);
                    else if(strcmp(content, "char")==0)
                        tk=addTk(CHAR);
                    else if(strcmp(content, "double")==0)
                        tk=addTk(DOUBLE);
                    else if(strcmp(content, "else")==0)
                        tk=addTk(ELSE);
                    else if(strcmp(content, "for")==0)
                        tk=addTk(FOR);
                    else if(strcmp(content, "if")==0)
                        tk=addTk(IF);
                    else if(strcmp(content, "int")==0)
                        tk=addTk(INT);
                    else if(strcmp(content, "return")==0)
                        tk=addTk(RETURN);
                    else if(strcmp(content, "struct")==0)
                        tk=addTk(STRUCT);
                    else if(strcmp(content, "void")==0)
                        tk=addTk(VOID);
                    else if(strcmp(content, "while")==0)
                        tk=addTk(WHILE);
                    else
                    {
                        tk=addTk(ID);
                        tk->text=content;
                    }
                    return tk->code;
                }
                break;
            case 3:
                {
                    if('0'<=ch && ch<='9')
                    {
                        current_index++;
                        ending_index++;
                    }
                    else if(ch=='.')
                    {
                        current_index++;
                        ending_index++;
                        state=8;
                    }
                    else if(ch=='e' || ch=='E')
                    {
                        current_index++;
                        ending_index++;
                        state=10;
                    } 
                    else
                        state=4;
                    
                }
                break;
            case 4: //final state CT_INT
                {
                    tk=addTk(CT_INT);
                    tk->i=createInt(createString(starting_index, ending_index+1));
                    return tk->code;
                }
                break;
            case 5:
                {
                    if('0'<=ch && ch<='7')
                    {
                        current_index++;
                        ending_index++;
                    } 
                    else if(ch=='x' || ch=='X')
                    {
                        current_index++;
                        ending_index++;
                        state=6;
                    }
                    else if(ch=='8' || ch=='9')
                    {
                        current_index++;
                        ending_index++;
                        state=7;
                    }
                    else if(ch=='.')
                    {
                        current_index++;
                        ending_index++;
                        state=8;
                    }
                    else
                        state=4;
                }
                break;
            case 6:
                {
                    if(('0'<=ch && ch<='9') || ('a'<=ch && ch<='f') || ('A'<=ch && ch<='F'))
                    {
                        current_index++;
                        ending_index++;
                    }
                    else
                        state=4;
                }
                break;
            case 7:
                {
                    if('0'<=ch && ch<='9')
                    {
                        current_index++;
                        ending_index++;
                    }
                    else if(ch=='.')
                    {
                        current_index++;
                        ending_index++;
                        state=8;
                    }
                }
                break;
            case 8:
                {
                    if('0'<=ch && ch<='9')
                    {
                        current_index++;
                        ending_index++;
                        state=9;
                    }
                }
                break;
            case 9:
                {
                    if('0'<=ch && ch<='9')
                    {
                        current_index++;
                        ending_index++;
                    }
                    else if(ch=='e' || ch=='E')
                    {
                        current_index++;
                        ending_index++;
                        state=10;
                    }
                    else
                        state=13;
                }
                break;
            case 10:
                {
                    if(ch=='-' || ch=='+')
                    {
                        current_index++;
                        ending_index++;
                    }
                    state=11;
                }
                break;
            case 11:
                {
                    if('0'<=ch && ch<='9')
                    {
                        current_index++;
                        ending_index++;
                        state=12;
                    }
                }
                break;
            case 12:
                {
                    if('0'<=ch && ch<='9')
                    {
                        current_index++;
                        ending_index++;
                    }
                    else
                        state=13;
                }
                break;
            case 13: //final state for CT_REAL
                {
                    tk=addTk(CT_REAL);
                    tk->r=createFloat(createString(starting_index, ending_index+1));
                    return tk->code;
                }
                break;
            case 14:
                {
                    if(ch=='\\')
                    {
                        current_index++;
                        ending_index++;
                        state=15;
                    }
                    else if(ch!='\'' && ch!='\\')
                    { 
                        current_index++;
                        ending_index++;
                        state=16;
                    }
                }
                break;
            case 15:
                {
                    if(strchr("abfnrtv'?\"\\0", ch)!=NULL)
                    {
                        current_index++;
                        ending_index++;
                        state=16;
                    }
                }
                break;
            case 16:
                {
                    if(ch=='\'')
                    {
                        current_index++;
                        ending_index++;
                        state=17;
                    }
                }
                break;
            case 17://final state for CT_CHAR
                {
                    tk=addTk(CT_CHAR);
                    char *copy = malloc(50);
                    copy=createString(starting_index, ending_index+1);
                    if(strlen(copy)==3)
                    {
                        tk->i=copy[1];
                    }
                    else if(strlen(copy)==4)
                    {
                        if(copy[2]=='a')
                            tk->i='\a';
                        else if(copy[2]=='b')
                            tk->i='\b';
                        else if(copy[2]=='f')
                            tk->i='\f';
                        else if(copy[2]=='n')
                            tk->i='\n';
                        else if(copy[2]=='r')
                            tk->i='\r';
                        else if(copy[2]=='t')
                            tk->i='\t';
                        else if(copy[2]=='v')
                            tk->i='\v';
                        else if(copy[2]=='\'')
                            tk->i='\'';
                        else if(copy[2]=='?')
                            tk->i='\?';
                        else if(copy[2]=='"')
                            tk->i='\"';
                        else if(copy[2]=='\\')
                            tk->i='\\';
                        else if(copy[2]=='0')
                            tk->i='\0';
                    }
                    free(copy);
                    return tk->code;
                }
                break;
            case 18:
                {
                    if(ch=='\\')
                    {
                        current_index++;
                        ending_index++;
                        state=19;
                    }
                    
                    else if(ch!='\'' && ch!='\\')
                    { 
                        current_index++;
                        ending_index++;
                        state=20;
                    }
                }
                break;
            case 19:
                {
                    if(strchr("abfnrtv'?\"\\0", ch)!=NULL)
                    {
                        current_index++;
                        ending_index++;
                        state=20;
                    }
                }
                break;
            case 20:
                {
                    if(ch=='\\')
                    {
                        current_index++;
                        ending_index++;
                        state=19;
                    }
                    else if(ch=='"')
                    {
                        current_index++;
                        ending_index++;
                        state=21;
                    }
                    else if(ch!='\'' && ch!='\\')
                    {
                        current_index++;
                        ending_index++;
                        state=20;
                    }
                }
                break;
            case 21://final state ct_string
                {
                    tk=addTk(CT_STRING);
                    tk->text=createString(starting_index, ending_index+1);
                    return tk->code;
                }
                break;
            case 22:
                {
                    if(ch=='*')
                    {
                        current_index++;
                        state=23;
                    }
                    else if(ch=='/')
                    {
                        current_index++;
                        state=35;
                    }
                    else
                    {
                        tk=addTk(DIV);
                        return tk->code;
                    }
                }
                break;
            case 23:
                {
                    current_index++;
                    if(ch=='*')
                    {
                        state=24;
                    }
                }
                break;
            case 24:
                {
                    current_index++;
                    if(ch=='/')
                    {
                        state=25;
                    }
                    else if(ch=='*')
                    {
                        state=24;
                    }
                    else
                    {
                        state=23;
                    }
                }
                break;
            case 25:
                {
                    printf("this was a comment");
                    return -1;
                }
                break;
            case 26:
                {
                    if(ch=='=')
                    {
                        current_index++;
                        tk=addTk(EQUALS);
                        return tk->code;
                    }
                    else
                    {
                        tk=addTk(ASSIGN);
                        return tk->code;
                    }
                }
                break;
            case 27: //comma final state
                {
                    tk=addTk(COMMA);
                    return tk->code;
                }
                break;
            case 28: //semicolon final state
                {
                    tk=addTk(SEMICOLON);
                    return tk->code;
                }
                break;
            case 29: //lpar final state
                {
                    tk=addTk(LPAR);
                    return tk->code;
                }
                break;
            case 30: //rpar final state
                {
                    tk=addTk(RPAR);
                    return tk->code;
                }
                break;
            case 31: //lbracket final state
                {
                    tk=addTk(LBRACKET);
                    return tk->code;
                }
                break;
            case 32: //rbracket final state
                {
                    tk=addTk(RBRACKET);
                    return tk->code;
                }
                break;
            case 33: //lacc final state
                {
                    tk=addTk(LACC);
                    return tk->code;
                }
                break;
            case 34: //racc final state
                {
                    tk=addTk(RACC);
                    return tk->code;
                }
                break;
            case 35:
                {
                    if(ch=='\n' || ch=='\r' || ch=='\0')
                    {
                        return -1;
                    }
                    else
                    {
                        current_index++;
                    }
                }
                break;
            case 36:
                {
                    tk=addTk(ADD);
                    return tk->code;
                }
                break;
            case 37:
                {
                    tk=addTk(SUB);
                    return tk->code;
                }
                break;
            case 38:
                {
                    tk=addTk(MUL);
                    return tk->code;
                }
                break;
            case 39:
                {
                    tk=addTk(DOT);
                    return tk->code;
                }
                break;
            case 40:
                {
                    if(ch=='&')
                    {
                        current_index++;
                        state=43;
                    }
                }
                break;
            case 41:
                {
                    if(ch=='|')
                    {
                        current_index++;
                        state=44;
                    }
                }
                break;
            case 42:
                {
                    if(ch=='=')
                    {
                        current_index++;
                        state=45;
                    }
                    else
                    {
                        tk=addTk(NOT);
                        return tk->code;
                    }
                }
                break;
            case 43:
                {
                    tk=addTk(AND);
                    return tk->code;
                }
                break;
            case 44:
                {
                    tk=addTk(OR);
                    return tk->code;
                }
                break;
            case 45:
                {
                    tk=addTk(NOTEQ);
                    return tk->code;
                }
                break;
            case 46:
                {
                    if(ch=='=')
                    {
                        current_index++;
                        state=47;
                    }
                    else
                    {
                        tk=addTk(LESS);
                        return tk->code;
                    }
                }
                break;
            case 47:
                {
                    tk=addTk(LESSEQ);
                    return tk->code;
                }
                break;
            case 48:
                {
                    if(ch=='=')
                    {
                        current_index++;
                        state=49;
                    }
                    else
                    {
                        tk=addTk(GREATER);
                        return tk->code;
                    }
                }
                break;
            case 49:
                {
                    tk=addTk(GREATEREQ);
                    return tk->code;
                }
                break;
        }
    }
}
