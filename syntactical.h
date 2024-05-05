#include <stdio.h>
#include <stdlib.h>

#include "lexical.h"

Token * tokens;

Token *expr(Token *start);
Token *stmCompound(Token *start);
Token *declVar(Token *start);
Token *funcArg(Token *start);

Token * exprPrimary(Token *start)
{
    Token *index=start;
    if((strcmp(enum_values[start->code], "CT_INT")==0)||
        (strcmp(enum_values[start->code], "CT_REAL")==0)||
        (strcmp(enum_values[start->code], "CT_CHAR")==0)||
        (strcmp(enum_values[start->code], "CT_STRING")==0))
    {
        return start->next;
        // printf("expresie\n");
    }
    else if(strcmp(enum_values[start->code], "LPAR")==0)
    {
        index=index->next;
        index=expr(index);
        if(strcmp(enum_values[start->code], "RPAR")==0)
        {
            index=index->next;
            return index;
        }
    }
    else if(strcmp(enum_values[start->code], "ID")==0)
    {
        index=index->next;
        if(strcmp(enum_values[index->code], "LPAR")==0)
        {
            index=index->next;
            Token *copy = index;
            index=expr(index);
            if(index!=copy)
            {
                
                while(1)
                {
                    if(strcmp(enum_values[index->code], "COMMA")==0)
                    {
                        index=index->next;
                        copy=index;
                        index=expr(index);
                        if(copy==index)
                            return start;
                    }
                    else
                        break;
                }
            }
            if(strcmp(enum_values[index->code], "RPAR")==0)
            {
                index=index->next; 
                return index;
            }

        }
        else
        {
            // index=index->next;
            return index;
        }
    }
    else
        return start;
}

int else_flag=0;

Token *stm(Token *start)
{
    Token *index=start;
    while(1)
    {
        Token *copy = index;
        copy=expr(copy);
        if(strcmp(enum_values[copy->code], "SEMICOLON")==0) //expr? SEMICOLON 
        {
            return copy->next;
        }
        copy=index;
        index=stmCompound(index);
        if(copy!=index)
        {
            printf("\nstm");
            return index;
        }
        if(strcmp(enum_values[index->code], "BREAK")==0) //BREAK SEMICOLON
        {
            index=index->next;
            if(strcmp(enum_values[index->code], "SEMICOLON")==0)
            {
                index=index->next;
                printf("stm\n");
                return index;
            }
            return start;
        }
        if(strcmp(enum_values[index->code], "RETURN")==0)//RETURN expr? SEMICOLON
        {
            index=index->next;
            index=expr(index);
            if(strcmp(enum_values[index->code], "SEMICOLON")==0)
            {
                index=index->next;
                printf("stm\n");
                return index;
            }
            return start;
        }
        if(strcmp(enum_values[index->code], "WHILE")==0) //WHILE LPAR expr RPAR stm
        {
            index=index->next;
            if(strcmp(enum_values[index->code], "LPAR")==0)
            {
                index=index->next;
                copy=index;
                index=expr(index);
                if(copy==index)
                    return start;
                if(strcmp(enum_values[index->code], "RPAR")==0)
                {
                    index=index->next;
                    else_flag=0;
                }
                else
                    return start;
            }
            else
                return start;
        }
        if(strcmp(enum_values[index->code], "FOR")==0) //FOR LPAR expr? SEMICOLON expr? SEMICOLON expr? RPAR stm
        {
            index=index->next;
            if(strcmp(enum_values[index->code], "LPAR")==0)
            {
                index=index->next;
                index=expr(index);
                if(strcmp(enum_values[index->code], "SEMICOLON")==0)
                {
                    index=index->next;
                    index=expr(index);
                    if(strcmp(enum_values[index->code], "SEMICOLON")==0)
                    {
                        index=index->next;
                        index=expr(index);
                        if(strcmp(enum_values[index->code], "RPAR")==0)
                        {
                            index=index->next;
                            else_flag=0;
                        }
                        else
                            return start;
                    }
                    else
                        return start;
                }
                else
                    return start;
            }
            else
                return start;
        }
        if(strcmp(enum_values[index->code], "IF")==0) //IF LPAR expr RPAR stm
        {
            index=index->next;
            if(strcmp(enum_values[index->code], "LPAR")==0)
            {
                index=index->next;
                copy=index;
                index=expr(index);
                if(copy==index)
                    return start;
                if(strcmp(enum_values[index->code], "RPAR")==0)
                {
                    index=index->next;
                    else_flag=1;
                }
                else
                    return start;
            }
            else
                return start;
        }
        if(strcmp(enum_values[index->code], "ELSE")==0 && else_flag==1) //( ELSE stm )?
        {
            index=index->next;
            else_flag=0;
        }
        if(index==start)
            return start;
    }
    
}

Token *stmCompound(Token* start)
{
    Token* index=start;
    if(strcmp(enum_values[index->code], "LACC")==0)
    {
        index=index->next;
        while(1)
        {
            Token *copy = index;
            index=declVar(index);
            index=stm(index);
            if(copy==index)
                break;
        }
        if(strcmp(enum_values[index->code], "RACC")==0)
        {
            index=index->next;
            printf("stm compound");
            return index;
        }
    }
    return start;
}

Token *declStruct(Token* start)
{
    Token *index=start;
    if(strcmp(enum_values[start->code], "STRUCT")==0)
    {
        index=index->next;
        if(strcmp(enum_values[start->code], "ID")==0)
        {
            index=index->next;
            if(strcmp(enum_values[start->code], "LACC")==0)
            {
                index=index->next;
                while(1)
                {
                    Token *copy=index;
                    index=declVar(index);
                    if(copy==index)
                        break;
                }
                if(strcmp(enum_values[start->code], "RACC")==0)
                {
                    index=index->next;
                    if(strcmp(enum_values[start->code], "SEMICOLON")==0)
                    {
                        index=index->next;
                        printf("decl struct");
                        return index;
                    }
                }
            }
        }
    }
    return start;
}

Token *declFunc(Token* start)
{
    Token *index=start;
    if(strcmp(enum_values[start->code], "INT")==0 ||
        strcmp(enum_values[start->code], "DOUBLE")==0 ||
        strcmp(enum_values[start->code], "CHAR")==0 ||
        strcmp(enum_values[start->code], "STRUCT")==0)
    {
        index=index->next;
        if(strcmp(enum_values[index->code], "MUL")==0)
        {
            index=index->next;
        }
    }
    else if(strcmp(enum_values[index->code], "VOID")==0)
        index=index->next;
    else
        return start;

    if(strcmp(enum_values[index->code], "ID")==0)
        index=index->next;
    else
        return start;

    if(strcmp(enum_values[index->code], "LPAR")==0)
        index=index->next;
    else
        return start;

    Token* copy = index;
    index=funcArg(index);
    if(copy!=index)
    {
        while(1)
        {
            copy=index;
            if(strcmp(enum_values[index->code], "COMMA")==0)
            {
                index=index->next;
                index=funcArg(index);
            }
            if(copy==index)
                break;
        }
    }

    if(strcmp(enum_values[index->code], "RPAR")==0)
        index=index->next;
    else
        return start;

    if(strcmp(enum_values[index->code], "SEMICOLON")==0)
    {
        index=index->next;
        printf("\nfunction header declaration");
    }

    copy=index;
    index=stmCompound(index);
    if(copy==index)
        return start;
    else
    {
        printf("\nfunction decl");
        return index;
    }

}

Token * exprPostfix(Token *start)
{
    Token *index = start;
    while(1)
    {
        index = exprPrimary(index);
        if(strcmp(enum_values[index->code], "DOT")==0)
        {
            index = index ->next;
            if(strcmp(enum_values[index->code], "ID")==0)
            {
                index=index->next;
                printf("expr postfix\n");
                return index;
            }
        }
        else if(strcmp(enum_values[index->code], "LBRACKET")==0)
        {
            index=index->next;
            index=expr(index);
            if(strcmp(enum_values[index->code], "RBRACKET")==0)
            {
                index=index->next;
                printf("expr postfix\n");
                return index;
            }
        }
        else{
            if(index==start)
                return start;
            else
                return index;
        }
    }
}

Token * exprUnary(Token *start)
{
    int state=0;
    Token *index=start;

    while(1)
    {
        index = exprPostfix(index);
        if(index!=start)
        {
            // printf("expr unary\n");
            return index;
        }
        
        if((strcmp(enum_values[index->code], "SUB")==0) ||
            (strcmp(enum_values[index->code], "NOT")==0))
            index = index->next;
        
        if(index==start)
            return start;
    }

}

Token * arrayDecl(Token *start)
{
    Token * reference = start;
    if(strcmp(enum_values[start->code], "LBRACKET")==0)
    {
        start = start -> next;
        start = exprPrimary(start);
        if(strcmp(enum_values[start->code], "RBRACKET")==0)
        {
            start = start -> next;
            printf("\na avut loc o declarare de vector");
            return start;
        }
    }
    return reference;
}

Token * typeName(Token *start)
{
    Token * index = start;
    if(strcmp(enum_values[start->code], "INT")==0 ||
        strcmp(enum_values[start->code], "DOUBLE")==0 ||
        strcmp(enum_values[start->code], "CHAR")==0 ||
        strcmp(enum_values[start->code], "STRUCT")==0)
    {
        index = index -> next;
        index = arrayDecl(index);
    }
    return index;

}

Token *declVar(Token * start)
{
    Token * reference = start;
    int state = 0;
    while(strcmp(enum_values[start->code], "SEMICOLON")!=0)
    {
        switch (state){
            case 0:
            {
                if(strcmp(enum_values[start->code], "INT")==0 ||
                    strcmp(enum_values[start->code], "DOUBLE")==0 ||
                    strcmp(enum_values[start->code], "CHAR")==0)
                {
                    start = start -> next;
                    state = 1;
                }
                else if(strcmp(enum_values[start->code], "STRUCT")==0)
                {
                    start=start->next;
                    state=3;
                }
                else
                {
                    return start;
                }
            }break;
            case 1:
            {
                if(strcmp(enum_values[start->code], "ID")==0)
                {
                    start = start -> next;
                    state = 2;
                }
            }break;
            case 2:
            {
                start = arrayDecl(start);
                if(strcmp(enum_values[start->code], "COMMA")==0)
                {
                    start = start -> next;
                    state = 1;
                }
            }break;
            case 3:
            {
                if(strcmp(enum_values[start->code], "ID")==0)
                {
                    start = start -> next;
                    state = 2;
                }
            }break;
        }
    }
    if(state == 2){
        printf("\nA avut loc o declarare de variabila");
        return start->next;
    }
    return reference;
}

Token *exprCast(Token *start)
{
    Token *index = start;
    while(1)
    {
        Token * copy = index;
        index = exprUnary(index);
        if(index!=copy)
        {
            printf("expr cast\n");
            return index;
        }

        if(strcmp(enum_values[index->code], "LPAR")==0)
        {
            index=index->next;
            index = typeName(index);
            if(strcmp(enum_values[index->code], "RPAR")==0)
                index=index->next;
            else
                return start;
        }
        else
            return start;
    }

    return index;
}

Token *exprMul(Token *start)
{
    Token *index = start;
    while(1)
    {
        index=exprCast(index);
        if(index!=start)
        {
            if((strcmp(enum_values[index->code], "MUL")==0)||
                (strcmp(enum_values[index->code], "DIV")==0))
                index=index->next;
            else
            {
                printf("expr mul\n");
                return index;
            }
        }
        else
            return start;
    }
}

Token *exprAdd(Token *start)
{
    Token *index = start;
    while(1)
    {
        index=exprMul(index);
        if(index!=start)
        {
            if((strcmp(enum_values[index->code], "ADD")==0)||
                (strcmp(enum_values[index->code], "SUB")==0))
                index=index->next;
            else
            {
                printf("expr sum\n");
                return index;
            }
        }
        else
            return start;
    }
}

Token *exprRel(Token *start)
{
    Token *index = start;
    while(1)
    {
        index=exprAdd(index);
        if(index!=start)
        {
            if((strcmp(enum_values[index->code], "LESS")==0)||
                (strcmp(enum_values[index->code], "LESSEQ")==0)||
                (strcmp(enum_values[index->code], "GREATER")==0)||
                (strcmp(enum_values[index->code], "GREATEREQ")==0))
                index=index->next;
            else
            {
                printf("expr rel\n");
                return index;
            }
        }
        else
            return start;
    }
}

Token *exprEq(Token *start)
{
    Token *index = start;
    while(1)
    {
        index=exprRel(index);
        if(index!=start)
        {
            if((strcmp(enum_values[index->code], "EQUAL")==0)||
                (strcmp(enum_values[index->code], "NOTEQ")==0))
                index=index->next;
            else
            {
                printf("expr eq\n");
                return index;
            }
        }
        else
            return start;
    }
}

Token *exprAnd(Token *start)
{
    Token *index = start;
    while(1)
    {
        index=exprEq(index);
        if(index!=start)
        {
            if(strcmp(enum_values[index->code], "AND")==0)
                index=index->next;
            else
            {
                printf("expr and\n");
                return index;
            }
        }
        else
            return start;
    }
}

Token *exprOr(Token *start)
{
    Token *index = start;
    while(1)
    {
        index=exprAnd(index);
        if(index!=start)
        {
            if(strcmp(enum_values[index->code], "OR")==0)
                index=index->next;
            else
            {
                printf("expr or\n");
                return index;
            }
        }
        else
            return start;
    }
}

Token *exprAssign(Token *start)
{
    Token *index = start;
    while(1)
    {
        Token *copy = index;
        index=exprOr(index);
        if(copy!=index)
        {
            if(strcmp(enum_values[index->code], "ASSIGN")!=0)
            {
                printf("expr assign\n");
                return index;
            }
        }
        else
        {
            index=exprUnary(index);
            if(strcmp(enum_values[index->code], "ASSIGN")==0)
                index=index->next;
        }
        if(start==index)
            return start;
    }
}

Token *expr(Token *start)
{
    return exprAssign(start);
}

Token *funcArg(Token *start)
{
    Token *index = start;
    if(strcmp(enum_values[start->code], "INT")==0 ||
                    strcmp(enum_values[start->code], "DOUBLE")==0 ||
                    strcmp(enum_values[start->code], "CHAR")==0 ||
                    strcmp(enum_values[start->code], "STRUCT")==0)
    {
        index = index -> next;
        if(strcmp(enum_values[index->code], "ID")==0)
        {
            index = index-> next;
            index = arrayDecl(index);
            return index;
        }
    }
    return start;
}

int syntactical_analysis(Token *tokens)
{
    Token * start = tokens;
    while(start->code != 1) 
    {
        Token *copy = start;
        start = declFunc(start);
        start = declVar(start);
        start = declStruct(start);
        if(copy==start)
            return 0;
    }
    return 1;
}

// int main()
// {
//     read_file("0.c");
//     lexical_analysis();
//     printf("\n");
//     syntactical_analysis();
// }