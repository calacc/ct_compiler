#include <stdio.h>
#include <stdlib.h>

#include "lexical.h"

enum{TB_INT,TB_DOUBLE,TB_CHAR,TB_STRUCT,TB_VOID};
enum{CLS_VAR,CLS_FUNC,CLS_EXTFUNC,CLS_STRUCT};
enum{MEM_GLOBAL,MEM_ARG,MEM_LOCAL};

struct _Symbol;
typedef struct _Symbol Symbol;

typedef struct{
    Symbol **begin; // the beginning of the symbols, or NULL
    Symbol **end; // the position after the last symbol
    Symbol **after; // the position after the allocated space
}Symbols;

typedef struct{
    int typeBase; // TB_*
    Symbol *s; // struct definition for TB_STRUCT
    int nElements; // >0 array of given size, 0=array without size, <0 non array
}Type;

typedef struct _Symbol{
    const char *name; // a reference to the name stored in a token
    int cls; // CLS_*
    int mem; // MEM_*
    Type type;
    int depth; // 0-global, 1-in function, 2... - nested blocks in function
    union{
        Symbols args; // used only of functions
        Symbols members; // used only for structs
    };
}Symbol;

Symbols symbols;
int crtDepth = 0;
Symbol *crtFunc = NULL, *crtStruct = NULL, *crtTk;

Symbol *addSymbol(Symbols *symbols,const char *name,int cls)
{
    Symbol *s;
    if(symbols->end==symbols->after)
    { // create more room
        int count=symbols->after-symbols->begin;
        int n=count*2; // double the room
        if(n==0)n=1; // needed for the initial case
        symbols->begin=(Symbol**) realloc(symbols->begin, n*sizeof(Symbol*));
        if(symbols->begin==NULL) err("not enough memory");
        symbols->end=symbols->begin+count;
        symbols->after=symbols->begin+n;
    }
    SAFEALLOC(s,Symbol)
    *symbols->end++=s;
    s->name=name;
    s->cls=cls;
    s->depth=crtDepth;
    return s;
}

Symbol *findSymbol(Symbols *symbols,const char *name)
{
    int n = symbols->end - symbols->begin;
    for(int i=n-1; i>=0; i--)
    {
        if(strcmp(name, symbols->begin[i]->name)==0)
            return symbols->begin[i];
    }
    return NULL;
}

void deleteSymbolsAfter(Symbols *symbols, Symbol *symbol)
{
    if (symbols->begin == NULL || symbols->end == NULL) {
        return;
    }

    Symbol **current = symbols->begin;
    while (current < symbols->end) {
        if (*current == symbol) {
            break;
        }
        current++;
    }

    if (current == symbols->end) {
        // Symbol not found, do nothing
        return;
    }

    // Move one past the found symbol
    current++;

    // Free all symbols after the found symbol
    while (current < symbols->end) {
        free(*current);
        current++;
    }

    // Update the end pointer to point to the symbol after the given one
    symbols->end = current - (current - symbols->begin);
}

void initSymbols(Symbols *symbols)
{
    symbols->begin=NULL;
    symbols->end=NULL;
    symbols->after=NULL;
}
////////////////////////////////////////////////////////////////////////////////

//TODO to define
Type getArithType(Type *s1,Type *s2);

Type createType(int typeBase,int nElements)
{
    Type t;
    t.typeBase=typeBase;
    t.nElements=nElements;
    return t;
}

typedef union{
    long int i; // int, char
    double d; // double
    const char *str; // char[]
}CtVal;
typedef struct{
    Type type; // type of the result
    int isLVal; // if it is a LVal
    int isCtVal; // if it is a constant value (int, real, char, char[])
    CtVal ctVal; // the constat value
}RetVal;

void my_cast(Type *dst,Type *src)
{
    if(src->nElements>-1){
        if(dst->nElements>-1){
            if(src->typeBase!=dst->typeBase)
            err("an array cannot be converted to an array of another type");
        }else{
            err("an array cannot be converted to a non-array");
        }
    }else{
        if(dst->nElements>-1){
            err("a non-array cannot be converted to an array");
        }
    }
    switch(src->typeBase){
        case TB_CHAR:
        case TB_INT:
        case TB_DOUBLE:
        switch(dst->typeBase){
            case TB_CHAR:
            case TB_INT:
            case TB_DOUBLE:
            return;
        }
        case TB_STRUCT:
        if(dst->typeBase==TB_STRUCT){
            if(src->s!=dst->s)
            err("a structure cannot be converted to another one");
            return;
        }
    }
    err("incompatible types");
}

///////////////////////
Token *expr(Token *start, RetVal *rv);
Token *stmCompound(Token *start);
Token *declVar(Token *start);
Token *funcArg(Token *start);


//TODO unde punem asta
// {
//     if(s->cls==CLS_FUNC||s->cls==CLS_EXTFUNC)
//         err("missing call for function %s",tkName->text);
// }
Token * exprPrimary(Token *start, RetVal *rv)
{
    RetVal *arg;
    SAFEALLOC(arg, RetVal)

    Token *index=start;
    if((strcmp(enum_values[start->code], "CT_INT")==0)||
        (strcmp(enum_values[start->code], "CT_REAL")==0)||
        (strcmp(enum_values[start->code], "CT_CHAR")==0)||
        (strcmp(enum_values[start->code], "CT_STRING")==0))
    {
        if(strcmp(enum_values[start->code], "CT_INT")==0)
        {
            Token *tki=index;
            rv->type=createType(TB_INT,-1);
            rv->ctVal.i=tki->i;
            rv->isCtVal=1;rv->isLVal=0;
        }
        else if(strcmp(enum_values[start->code], "CT_REAL")==0)
        {
            Token *tkr=index;
            rv->type=createType(TB_DOUBLE,-1);
            rv->ctVal.d=tkr->r;
            rv->isCtVal=1;rv->isLVal=0;
        }
        else if(strcmp(enum_values[start->code], "CT_CHAR")==0)
        {
            Token *tkc=index;
            rv->type=createType(TB_CHAR,-1);
            rv->ctVal.i=tkc->i;
            rv->isCtVal=1;rv->isLVal=0;
        }
        else if(strcmp(enum_values[start->code], "CT_STRING")==0)
        {
            Token *tks=index;
            rv->type=createType(TB_CHAR,0);
            rv->ctVal.str=tks->text;
            rv->isCtVal=1;rv->isLVal=0;
        }
        return start->next;
        // printf("expresie\n");
    }
    else if(strcmp(enum_values[start->code], "LPAR")==0)
    {
        index=index->next;
        index=expr(index, arg);
        if(strcmp(enum_values[start->code], "RPAR")==0)
        {
            index=index->next;
            return index;
        }
        else
            err("right paranthesis missing from primary expression");
    }
    else if(strcmp(enum_values[start->code], "ID")==0)
    {
        Token *tkName;
        SAFEALLOC(tkName, Token)
        Symbol *s=findSymbol(&symbols,tkName->text);
        if(!s)err("undefined symbol %s",tkName->text);
        rv->type=s->type;
        rv->isCtVal=0;
        rv->isLVal=1;

        index=index->next;
        if(strcmp(enum_values[index->code], "LPAR")==0)
        {
            Symbol **crtDefArg=s->args.begin;
            if(s->cls!=CLS_FUNC&&s->cls!=CLS_EXTFUNC)
                err("call of the non-function %s",tkName->text);

            index=index->next;
            Token *copy = index;

            index=expr(index, arg);
            if(index!=copy)
            {
                if(crtDefArg==s->args.end)err("too many arguments in call");
                my_cast(&(*crtDefArg)->type,&arg->type);
                crtDefArg++;
                while(1)
                {
                    if(strcmp(enum_values[index->code], "COMMA")==0)
                    {
                        index=index->next;
                        copy=index;
                        index=expr(index, rv);
                        if(copy==index)
                            return start;
                        if(crtDefArg==s->args.end)err("too many arguments in call");
                        my_cast(&(*crtDefArg)->type,&arg->type);
                        crtDefArg++;
                    }
                    else
                        break;
                }
            }
            if(strcmp(enum_values[index->code], "RPAR")==0)
            {
                if(crtDefArg!=s->args.end) err("too few arguments in call");
                rv->type=s->type;
                rv->isCtVal=rv->isLVal=0;

                index=index->next; 
                return index;
            }
            else 
                err("right paranthesis missing from array declaration");

        }
        else
        {
            return index;
        }
    }
    return start;
}

int else_flag=0;

Token *stm(Token *start)
{
    Token *index=start;
    while(1)
    {
        Token *copy = index;
        RetVal *rv;
        SAFEALLOC(rv, RetVal)
        copy=expr(copy, rv);
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
            else 
                err("semicolon missing from break statement");
        }
        if(strcmp(enum_values[index->code], "RETURN")==0)//RETURN expr? SEMICOLON
        {
            index=index->next;
            index=expr(index, rv);
            if(crtFunc->type.typeBase==TB_VOID)
                err("a void function cannot return a value");
            my_cast(&crtFunc->type,&rv->type);
            if(strcmp(enum_values[index->code], "SEMICOLON")==0)
            {
                index=index->next;
                printf("stm\n");
                return index;
            }
            else 
                err("semicolon missing from return statement");
        }
        if(strcmp(enum_values[index->code], "WHILE")==0) //WHILE LPAR expr RPAR stm
        {
            index=index->next;
            if(strcmp(enum_values[index->code], "LPAR")==0)
            {
                index=index->next;
                copy=index;
                index=expr(index, rv);
                if(rv->type.typeBase==TB_STRUCT)
                    err("a structure cannot be logically tested");
                if(copy==index)
                    err("condition missing from while statement");
                if(strcmp(enum_values[index->code], "RPAR")==0)
                {
                    index=index->next;
                    else_flag=0;
                }
                else 
                    err("right paranthesis missing from while statement");
            }
            else 
                err("left paranthesis missing from while statement");
        }
        if(strcmp(enum_values[index->code], "FOR")==0) //FOR LPAR expr? SEMICOLON expr? SEMICOLON expr? RPAR stm
        {
            RetVal *rv1, *rv2, *rv3;
            SAFEALLOC(rv1, RetVal)
            SAFEALLOC(rv2, RetVal)
            SAFEALLOC(rv3, RetVal)

            index=index->next;
            if(strcmp(enum_values[index->code], "LPAR")==0)
            {
                index=index->next;
                index=expr(index, rv1);
                if(strcmp(enum_values[index->code], "SEMICOLON")==0)
                {
                    index=index->next;
                    index=expr(index, rv2);
                    if(rv2->type.typeBase==TB_STRUCT)
                        err("a structure cannot be logically tested");
                    if(strcmp(enum_values[index->code], "SEMICOLON")==0)
                    {
                        index=index->next;
                        index=expr(index, rv3);
                        if(strcmp(enum_values[index->code], "RPAR")==0)
                        {
                            index=index->next;
                            else_flag=0;
                        }
                        else 
                            err("right paranthesis missing from for statement");
                    }
                    else 
                        err("second semicolon missing from for statement");
                }
                else 
                    err("first semicolon missing from for statement");
            }
            else 
                err("left paranthesis missing from for statement");
        }
        if(strcmp(enum_values[index->code], "IF")==0) //IF LPAR expr RPAR stm
        {
            index=index->next;
            if(strcmp(enum_values[index->code], "LPAR")==0)
            {
                index=index->next;
                copy=index;
                index=expr(index, rv);
                if(rv->type.typeBase==TB_STRUCT)
                    err("a structure cannot be logically tested");
                if(copy==index)
                    err("condition missing from if statement");
                if(strcmp(enum_values[index->code], "RPAR")==0)
                {
                    index=index->next;
                    else_flag=1;
                }
                else 
                    err("right paranthesis missing from if statement");
            }
            else 
                err("left paranthesis missing from if statement");
        }
        if(strcmp(enum_values[index->code], "ELSE")==0) //( ELSE stm )?
        {
            if(else_flag==1)
            {
                index=index->next;
                else_flag=0;
            }
            else 
                err("else statement without if");
        }
        if(index==start)
            return start;
    }
    
}

Token *stmCompound(Token* start)
{
    Token* index=start;
    Symbol *start_s=symbols.end[-1];

    if(strcmp(enum_values[index->code], "LACC")==0)
    {
        crtDepth++;
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

            crtDepth--;
            deleteSymbolsAfter(&symbols,start_s);
            return index;
        }
        else 
            err("} missing from compound statement");
    }
    return start;
}

Token *declStruct(Token* start)
{
    Token *index=start;
    if(strcmp(enum_values[index->code], "STRUCT")==0)
    {
        index=index->next;
        if(strcmp(enum_values[index->code], "ID")==0)
        {
            Token * tkName = start;
            index=index->next;
            if(strcmp(enum_values[index->code], "LACC")==0)
            {
                index=index->next;

                if(findSymbol(&symbols,tkName->text))
                    err("symbol redefinition in declStruct");//err(index,"symbol redefinition: %s",tkName->text);
                crtStruct=addSymbol(&symbols,tkName->text,CLS_STRUCT);
                initSymbols(&crtStruct->members);

                while(1)
                {
                    Token *copy=index;
                    index=declVar(index);
                    if(copy==index)
                        break;
                }
                if(strcmp(enum_values[index->code], "RACC")==0)
                {
                    index=index->next;
                    if(strcmp(enum_values[index->code], "SEMICOLON")==0)
                    {
                        index=index->next;
                        crtStruct=NULL;
                        printf("decl struct");
                        return index;
                    }
                    else 
                        err("semicolon missing from struct declaration");
                }
                else 
                    err("} missing from struct declaration");
            }
            else 
                err("{ missing from struct declaration");
        }
        else 
            err("id missing from struct declaration");
    }
    return start;
}

Token *declFunc(Token* start)
{
    Token *index=start, *tkName;
    Type *t;
    SAFEALLOC(t,Type)
    SAFEALLOC(tkName,Token)

    if(strcmp(enum_values[start->code], "INT")==0 ||
        strcmp(enum_values[start->code], "DOUBLE")==0 ||
        strcmp(enum_values[start->code], "CHAR")==0 ||
        strcmp(enum_values[start->code], "STRUCT")==0)
    {

        if(strcmp(enum_values[start->code], "INT")==0 ||
            strcmp(enum_values[start->code], "DOUBLE")==0 ||
            strcmp(enum_values[start->code], "CHAR")==0)
        {
            if(strcmp(enum_values[start->code], "INT")==0)
                t-> typeBase = TB_INT;
            else if(strcmp(enum_values[start->code], "DOUBLE")==0)
                t->typeBase = TB_DOUBLE;
            else
                t->typeBase = TB_CHAR;
            start = start -> next;
        }
        else if(strcmp(enum_values[start->code], "STRUCT")==0)
        {
            start=start->next;
            t->typeBase=TB_STRUCT;
            tkName->text=start->text;
            Symbol *s=findSymbol(&symbols,tkName->text);
            if(s==NULL) err("undefined symbol in declFunc");//err("undefined symbol: %s",tkName->text);
            if(s->cls!=CLS_STRUCT) err("is not a struct in declFunc");//err("%s is not a struct",tkName->text);
        }

        index=index->next;

        if(strcmp(enum_values[index->code], "MUL")==0)
        {
            index=index->next;
            t->nElements = 0;
        }
        else
        {
            t->nElements = -1;
        }
    }
    else if(strcmp(enum_values[index->code], "VOID")==0)
    {
        index=index->next;
        t->typeBase=TB_VOID;
    }
    else
        return start;

    if(strcmp(enum_values[index->code], "ID")==0)
    {
        tkName->text=index->text;
        index=index->next;
    }
    else
        return start;

    if(strcmp(enum_values[index->code], "LPAR")==0)
        index=index->next;
    else
        return start;

    if(findSymbol(&symbols,tkName->text))
        err("symbol redefinition in declFunc");//err("symbol redefinition: %s",tkName->text);
    crtFunc=addSymbol(&symbols,tkName->text,CLS_FUNC);
    initSymbols(&crtFunc->args);
    crtFunc->type=*t;
    crtDepth++;    

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
        err("right paranthesis missing from function declaration");

    crtDepth--; 

    if(strcmp(enum_values[index->code], "SEMICOLON")==0)
    {
        index=index->next;
        printf("\nfunction header declaration");
    }

    copy=index;
    index=stmCompound(index);

    deleteSymbolsAfter(&symbols,crtFunc); 
    crtFunc=NULL;

    if(copy==index)
        return start;
    else
    {
        printf("\nfunction decl");
        return index;
    }

}

Token * exprPostfix(Token *start, RetVal *rv)
{
    Token *index = start;
    while(1)
    {
        index = exprPrimary(index, rv);
        if(strcmp(enum_values[index->code], "DOT")==0)
        {
            index = index ->next;
            if(strcmp(enum_values[index->code], "ID")==0)
            {
                Token *tkName = index;
                Symbol      *sStruct=rv->type.s;
                Symbol      *sMember=findSymbol(&sStruct->members,tkName->text);
                if(!sMember)
                    err("struct %s does not have a member %s",sStruct->name,tkName->text);
                rv->type=sMember->type;
                rv->isLVal=1;
                rv->isCtVal=0;
                
                index=index->next;
                printf("expr postfix\n");
                return index;
            }
        }
        else if(strcmp(enum_values[index->code], "LBRACKET")==0)
        {
            index=index->next;
            RetVal *rve;
            SAFEALLOC(rve, RetVal)

            Token *copy=index;
            index=expr(index, rve);
            if(copy!=index)
            {
                if(rv->type.nElements<0)err("only an array can be indexed");
                Type typeInt=createType(TB_INT,-1);
                my_cast(&typeInt,&rve->type);
                rv->type=rv->type;
                rv->type.nElements=-1;
                rv->isLVal=1;
                rv->isCtVal=0;
            }

            if(strcmp(enum_values[index->code], "RBRACKET")==0)
            {
                index=index->next;
                printf("expr postfix\n");
                return index;
            }
            else 
                err("right bracket missing from postfix expression");
        }
        else{
            if(index==start)
                return start;
            else
                return index;
        }
    }
}

Token * exprUnary(Token *start, RetVal *rv)
{
    Token *index=start;
    Token *tkop;
    SAFEALLOC(tkop, Token)

    while(1)
    {
        index = exprPostfix(index, rv);
        if(index!=start)
        {
            // printf("expr unary\n");
            return index;
        }
        
        if((strcmp(enum_values[index->code], "SUB")==0) ||
            (strcmp(enum_values[index->code], "NOT")==0))
        {
            tkop = index;
            index = index->next;
        }

        if(tkop->code==SUB){
            if(rv->type.nElements>=0) err("unary '-' cannot be applied to an array");
            if(rv->type.typeBase==TB_STRUCT)
                err("unary '-' cannot be applied to a struct");
        }
        else{  // NOT
            if(rv->type.typeBase==TB_STRUCT)err("'!' cannot be applied to a struct");
            rv->type=createType(TB_INT,-1);
        }
        rv->isCtVal=rv->isLVal=0;
        
        
        if(index==start)
            return start;
    }

}

Token * arrayDecl(Token *start, Type *t)
{
    Token * reference = start;
    RetVal *rv;
    SAFEALLOC(rv, RetVal)
    if(strcmp(enum_values[start->code], "LBRACKET")==0)
    {   
        start = start -> next;
        Token *copy = start;
        start = exprPrimary(start, rv);
        if(start!=copy)
        {
            if(!rv->isCtVal)err("the array size is not a constant");
            if(rv->type.typeBase!=TB_INT)err("the array size is not an integer");
            t->nElements=rv->ctVal.i;
        }
        t->nElements=0;
        if(strcmp(enum_values[start->code], "RBRACKET")==0)
        {
            start = start -> next;
            printf("\na avut loc o declarare de vector");
            return start;
        }
        else
            err("right bracket missing from array declaration");
    }
    return reference;
}

Token * typeName(Token *start, Type *t)
{
    Token * index = start;
    // Type *t;
    // SAFEALLOC(t,Type)

    if(strcmp(enum_values[start->code], "INT")==0 ||
        strcmp(enum_values[start->code], "DOUBLE")==0 ||
        strcmp(enum_values[start->code], "CHAR")==0 ||
        strcmp(enum_values[start->code], "STRUCT")==0)
    {
        index = index -> next;
        if(strcmp(enum_values[start->code], "INT")==0 ||
            strcmp(enum_values[start->code], "DOUBLE")==0 ||
            strcmp(enum_values[start->code], "CHAR")==0)
        {
            if(strcmp(enum_values[start->code], "INT")==0)
                t-> typeBase = TB_INT;
            else if(strcmp(enum_values[start->code], "DOUBLE")==0)
                t->typeBase = TB_DOUBLE;
            else
                t->typeBase = TB_CHAR;
            start = start -> next;
        }
        else if(strcmp(enum_values[start->code], "STRUCT")==0)
        {
            start=start->next;
            t->typeBase=TB_STRUCT;
            Token *tkName;
            SAFEALLOC(tkName,Token)

            tkName->text=start->text;
            Symbol *s=findSymbol(&symbols,tkName->text);
            if(s==NULL) err("undefined symbol in typeName");//err("undefined symbol: %s",tkName->text);
            if(s->cls!=CLS_STRUCT) err("is not a struct in typeName");//err("%s is not a struct",tkName->text);
        }

        Token *copy = index;
        index = arrayDecl(index, t);
        if(index==copy)
            t->nElements = -1;
    }
    return index;

}

void addVar(Token *tkName,Type *t)
{
    Symbol *s=NULL;
    if(crtStruct)
    {
        if(findSymbol(&crtStruct->members,tkName->text))
            err("symbol redefinition in addVar");//err("symbol redefinition: %s",tkName->text);
        s=addSymbol(&crtStruct->members,tkName->text,CLS_VAR);
    }
    else if(crtFunc)
    {
        s=findSymbol(&symbols,tkName->text);
        if(s&&s->depth==crtDepth)
            err("symbol redefinition in addVar");//err("symbol redefinition: %s",tkName->text);
        s=addSymbol(&symbols,tkName->text,CLS_VAR);
        s->mem=MEM_LOCAL;
    }
    else
    {
        if(findSymbol(&symbols,tkName->text))
            err("symbol redefinition in addVar");//err("symbol redefinition: %s",tkName->text);
        s=addSymbol(&symbols,tkName->text,CLS_VAR);   
        s->mem=MEM_GLOBAL;
    }
    s->type=*t;
}

Token *declVar(Token * start)
{
    Token * reference = start;
    int state = 0;
    Type *t;
    Token *tkName;
    SAFEALLOC(t,Type)
    SAFEALLOC(tkName,Token)
    
    while(strcmp(enum_values[start->code], "SEMICOLON")!=0)
    {
        switch (state){
            case 0:
            {
                if(strcmp(enum_values[start->code], "INT")==0 ||
                    strcmp(enum_values[start->code], "DOUBLE")==0 ||
                    strcmp(enum_values[start->code], "CHAR")==0)
                {
                    if(strcmp(enum_values[start->code], "INT")==0)
                        t-> typeBase = TB_INT;
                    else if(strcmp(enum_values[start->code], "DOUBLE")==0)
                        t->typeBase = TB_DOUBLE;
                    else
                        t->typeBase = TB_CHAR;
                    start = start -> next;
                    state = 1;
                }
                else if(strcmp(enum_values[start->code], "STRUCT")==0)
                {
                    t->typeBase=TB_STRUCT;
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
                    tkName=start;
                    start = start -> next;
                    state = 2;
                    addVar(tkName, t);
                }
            }break;
            case 2:
            {
                Token *copy = start;
                start = arrayDecl(start, t); //TODO de vazut
                if(copy==start)
                    t->nElements=-1;

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
                    tkName = start;
                    start = start -> next;
                    state = 2;
                    addVar(tkName, t);
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

//TODO de lamurit...
Token *exprCast(Token *start, RetVal *rv)
{
    Token *index = start;
    Type *t;
    SAFEALLOC(t, Type)
    RetVal *rve;
    SAFEALLOC(rve, RetVal)

    while(1)
    {
        Token * copy = index;
        index = exprUnary(index, rv);
        if(index!=copy)
        {
            printf("expr cast\n");

            my_cast(t,&rve->type);
            rv->type=*t;
            rv->isCtVal=rv->isLVal=0;

            return index;
        }

        if(strcmp(enum_values[index->code], "LPAR")==0)
        {
            index=index->next;
            index = typeName(index, t);
            if(strcmp(enum_values[index->code], "RPAR")==0)
                index=index->next;
            else 
                err("right paranthesis missing from expression cast");
        }
        else
            return start;
    }

    return index;
}

Token *exprMul(Token *start, RetVal *rv)
{
    Token *index = start, *tkop;
    RetVal *rve;
    SAFEALLOC(rve, RetVal)
    SAFEALLOC(tkop, Token)


    while(1)
    {
        index=exprCast(index, rve);
        if(index!=start)
        {
            if((strcmp(enum_values[index->code], "MUL")==0)||
                (strcmp(enum_values[index->code], "DIV")==0))
            {
                tkop = index;
                index=index->next;
            }
            else
            {
                if(rv->type.nElements>-1||rve->type.nElements>-1)
                    err("an array cannot be multiplied or divided");
                if(rv->type.typeBase==TB_STRUCT||rve->type.typeBase==TB_STRUCT)
                    err("a structure cannot be multiplied or divided");
                //TODO de decomentat
                //rv->type=getArithType(&rv->type,&rve->type);
                rv->isCtVal=rv->isLVal=0;

                printf("expr mul\n");
                return index;
            }
        }
        else
            return start;
    }
}

Token *exprAdd(Token *start, RetVal *rv)
{
    RetVal *rve;
    Token *index = start, *tkop;
    SAFEALLOC(rve, RetVal)
    SAFEALLOC(tkop, Token)
    while(1)
    {
        index=exprMul(index, rve);
        if(index!=start)
        {
            if((strcmp(enum_values[index->code], "ADD")==0)||
                (strcmp(enum_values[index->code], "SUB")==0))
            {
                tkop=index;
                index=index->next;
            }
            else
            {
                if(rv->type.nElements>-1||rve->type.nElements>-1)
                    err("an array cannot be added or subtracted");
                if(rv->type.typeBase==TB_STRUCT||rve->type.typeBase==TB_STRUCT)
                    err("a structure cannot be added or subtracted");
                //TODO de decomentat
                //rv->type=getArithType(&rv->type,&rve->type);
                rv->isCtVal=rv->isLVal=0;        
                printf("expr sum\n");
                return index;
            }
        }
        else
            return start;
    }
}

Token *exprRel(Token *start, RetVal *rv)
{
    Token *index = start, *tkop;
    RetVal *rve;
    SAFEALLOC(tkop, Token)
    SAFEALLOC(rve, RetVal)
    while(1)
    {
        index=exprAdd(index, rve);
        if(index!=start)
        {
            if((strcmp(enum_values[index->code], "LESS")==0)||
                (strcmp(enum_values[index->code], "LESSEQ")==0)||
                (strcmp(enum_values[index->code], "GREATER")==0)||
                (strcmp(enum_values[index->code], "GREATEREQ")==0))
            {
                tkop=index;
                index=index->next;
            }
            else
            {
                 if(rv->type.nElements>-1||rve->type.nElements>-1)
                    err("an array cannot be compared");
                if(rv->type.typeBase==TB_STRUCT||rve->type.typeBase==TB_STRUCT)
                    err("a structure cannot be compared");
                rv->type=createType(TB_INT,-1);
                rv->isCtVal=rv->isLVal=0;

                printf("expr rel\n");
                return index;
            }
        }
        else
            return start;
    }
}

Token *exprEq(Token *start, RetVal *rv)
{
    Token *index = start, *tkop;
    RetVal *rve;
    SAFEALLOC(tkop, Token)
    SAFEALLOC(rve, RetVal)
    while(1)
    {
        index=exprRel(index, rve);
        if(index!=start)
        {
            if((strcmp(enum_values[index->code], "EQUAL")==0)||
                (strcmp(enum_values[index->code], "NOTEQ")==0))
            {
                tkop=index;
                index=index->next;
            }
            else
            {
                 if(rv->type.typeBase==TB_STRUCT||rve->type.typeBase==TB_STRUCT)
                    err("a structure cannot be compared");
                rv->type=createType(TB_INT,-1);
                rv->isCtVal=rv->isLVal=0;
                printf("expr eq\n");
                return index;
            }
        }
        else
            return start;
    }
}

Token *exprAnd(Token *start, RetVal *rv)
{
    Token *index = start, *tkop;
    RetVal *rve;
    SAFEALLOC(rve, RetVal)
    SAFEALLOC(tkop, Token)
    while(1)
    {
        index=exprEq(index, rve);
        if(index!=start)
        {
            if(strcmp(enum_values[index->code], "AND")==0)
            {
                tkop=index;
                index=index->next;
            }
            else
            {
                if(rv->type.typeBase==TB_STRUCT||rve->type.typeBase==TB_STRUCT)
                    err("a structure cannot be logically tested");
                rv->type=createType(TB_INT,-1);
                rv->isCtVal=rv->isLVal=0;

                printf("expr and\n");
                return index;
            }
        }
        else
            return start;
    }
}

Token *exprOr(Token *start, RetVal *rv)
{
    Token *index = start, *tkop;
    RetVal *rve;
    SAFEALLOC(rve, RetVal)
    SAFEALLOC(tkop, Token)
    while(1)
    {
        index=exprAnd(index, rve);
        if(index!=start)
        {
            if(strcmp(enum_values[index->code], "OR")==0)
            {
                tkop=index;
                index=index->next;
            }
            else
            {
                 if(rv->type.typeBase==TB_STRUCT||rve->type.typeBase==TB_STRUCT)
                    err("a structure cannot be logically tested");
                rv->type=createType(TB_INT,-1);
                rv->isCtVal=rv->isLVal=0;
                printf("expr or\n");
                return index;
            }
        }
        else
            return start;
    }
}

//TODO de lamurit frt
Token *exprAssign(Token *start, RetVal* rv)
{
    Token *index = start, *tkop;
    RetVal *rve;
    SAFEALLOC(rve, RetVal)
    SAFEALLOC(tkop, Token)
    while(1)
    {
        Token *copy = index;
        index=exprOr(index, rve);
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
            index=exprUnary(index, rv);
            if(strcmp(enum_values[index->code], "ASSIGN")==0)
                index=index->next;
        }
        if(start==index)
            return start;
    }
}

Token *expr(Token *start, RetVal *rv)
{
    return exprAssign(start, rv);
}

Token *funcArg(Token *start)
{
    Token *index = start, *tkName;
    Type *t;

    SAFEALLOC(t,Type)
    SAFEALLOC(tkName,Token)

    if(strcmp(enum_values[start->code], "INT")==0 ||
                    strcmp(enum_values[start->code], "DOUBLE")==0 ||
                    strcmp(enum_values[start->code], "CHAR")==0 ||
                    strcmp(enum_values[start->code], "STRUCT")==0)
    {
        index = index -> next;

        if(strcmp(enum_values[start->code], "INT")==0 ||
            strcmp(enum_values[start->code], "DOUBLE")==0 ||
            strcmp(enum_values[start->code], "CHAR")==0)
        {
            if(strcmp(enum_values[start->code], "INT")==0)
                t-> typeBase = TB_INT;
            else if(strcmp(enum_values[start->code], "DOUBLE")==0)
                t->typeBase = TB_DOUBLE;
            else
                t->typeBase = TB_CHAR;
            start = start -> next;
        }
        else if(strcmp(enum_values[start->code], "STRUCT")==0)
        {
            start=start->next;
            t->typeBase=TB_STRUCT;
            tkName->text=start->text;
            Symbol *s=findSymbol(&symbols,tkName->text);
            if(s==NULL) err("undefined symbol in funcArg");//err("undefined symbol: %s",tkName->text);
            if(s->cls!=CLS_STRUCT) err("is not a struct in funcArg");//err("%s is not a struct",tkName->text);
        }

        if(strcmp(enum_values[index->code], "ID")==0)
        {
            tkName->text = index->text;
            index = index-> next;

            Token *copy = index;
            index = arrayDecl(index, t);
            if(index==copy)
                t->nElements = -1;

            Symbol  *s=addSymbol(&symbols,tkName->text,CLS_VAR);
            s->mem=MEM_ARG;
            s->type=*t;
            s=addSymbol(&crtFunc->args,tkName->text,CLS_VAR);
            s->mem=MEM_ARG;
            s->type=*t;

            return index;
        }
        else 
            err("missing function argument id");
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