#include <stdio.h>
#include <stdlib.h>
#include <string.h>

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
    s->type.nElements = -2;
    return s;
}

Symbol *findSymbol(Symbols *symbols,const char *name)
{
    int n = symbols->end - symbols->begin;
    for(int i=n-1; i>=0; i--)
    {
        Symbol *s = symbols->begin[i];
        if(strcmp(name, symbols->begin[i]->name)==0)
            return symbols->begin[i];
    }
    return NULL;
}

void deleteSymbolsAfter(Symbols *symbols, Symbol *symbol)
{
    int n = symbols->end - symbols->begin;
    Symbol *ultimu_simbol = symbols->begin[n-1];
    if (symbols->begin == NULL || symbols->end == NULL) {
        return;
    }

    Symbol **current = symbols->begin, **new_last;
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
    new_last=current;

    // Free all symbols after the found symbol
    while (current < symbols->end) {
        free(*current);
        current++;
    }

    // Update the end pointer to point to the symbol after the given one
    symbols->end = new_last;
}

void initSymbols(Symbols *symbols)
{
    symbols->begin=NULL;
    symbols->end=NULL;
    symbols->after=NULL;
}
////////////////////////////////////////////////////////////////////////////////

//TODO to define
Type getArithType(Type *s1,Type *s2)
{
    printf("s1 type:%d s2 type:%d", s1->typeBase, s2->typeBase);
    return *s1;
}

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
            err("an array cannot be converted to an array of another type my_cast");
        }else{
            err("an array cannot be converted to a non-array my_cast");
        }
    }else{
        if(dst->nElements>-1){
            err("a non-array cannot be converted to an array my_cast");
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
            err("a structure cannot be converted to another one my_cast");
            return;
        }
    }
    err("incompatible types my_cast");
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
        printf("expresie\n");
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
            err("right paranthesis missing from primary expression exprPrimary");
    }
    else if(strcmp(enum_values[start->code], "ID")==0)
    {
        Token *tkName=start;
        Symbol *s=findSymbol(&symbols,tkName->text);
        if(!s)err("undefined symbol %s exprPrimary",tkName->text);
        rv->type=s->type;
        printf("exprPrimary %d", rv->type.nElements);
        rv->isCtVal=0;
        rv->isLVal=1;

        index=index->next;
        if(strcmp(enum_values[index->code], "LPAR")==0)
        {
            Symbol **crtDefArg=s->args.begin;
            if(s->cls!=CLS_FUNC&&s->cls!=CLS_EXTFUNC)
                err("call of the non-function %s exprPrimary",tkName->text);

            index=index->next;
            Token *copy = index;

            index=expr(index, arg);
            if(index!=copy)
            {
                if(crtDefArg==s->args.end)err("too many arguments in call exprPrimary");
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
                        if(crtDefArg==s->args.end)err("too many arguments in call exprPrimary");
                        my_cast(&(*crtDefArg)->type,&arg->type);
                        crtDefArg++;
                    }
                    else
                        break;
                }
            }
            if(strcmp(enum_values[index->code], "RPAR")==0)
            {
                if(crtDefArg!=s->args.end) err("too few arguments in call exprPrimary");
                rv->type=s->type;
                rv->isCtVal=rv->isLVal=0;

                index=index->next; 
                return index;
            }
            else 
                err("right paranthesis missing from array declaration exprPrimary");

        }
        else
        {
            if(s->cls==CLS_FUNC||s->cls==CLS_EXTFUNC)
                err("missing call for function %s exprPrimary",tkName->text);
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
                err("semicolon missing from break statement stm");
        }
        if(strcmp(enum_values[index->code], "RETURN")==0)//RETURN expr? SEMICOLON
        {
            index=index->next;
            index=expr(index, rv);
            if(crtFunc->type.typeBase==TB_VOID)
                err("a void function cannot return a value stm");
            my_cast(&crtFunc->type,&rv->type);
            if(strcmp(enum_values[index->code], "SEMICOLON")==0)
            {
                index=index->next;
                printf("return stm\n");
                return index;
            }
            else 
                err("semicolon missing from return statement stm");
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
                    err("a structure cannot be logically tested stm");
                if(copy==index)
                    err("condition missing from while statement stm");
                if(strcmp(enum_values[index->code], "RPAR")==0)
                {
                    index=index->next;
                    else_flag=0;
                }
                else 
                    err("right paranthesis missing from while statement stm");
            }
            else 
                err("left paranthesis missing from while statement stm");
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
                        err("a structure cannot be logically tested stm");
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
                            err("right paranthesis missing from for statement stm");
                    }
                    else 
                        err("second semicolon missing from for statement stm");
                }
                else 
                    err("first semicolon missing from for statement stm");
            }
            else 
                err("left paranthesis missing from for statement stm");
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
                    err("a structure cannot be logically tested stm");
                if(copy==index)
                    err("condition missing from if statement stm");
                if(strcmp(enum_values[index->code], "RPAR")==0)
                {
                    index=index->next;
                    else_flag=1;
                }
                else 
                    err("right paranthesis missing from if statement stm");
            }
            else 
                err("left paranthesis missing from if statement stm");
        }
        if(strcmp(enum_values[index->code], "ELSE")==0) //( ELSE stm )?
        {
            if(else_flag==1)
            {
                index=index->next;
                else_flag=0;
            }
            else 
                err("else statement without if stm");
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
                Symbol *sStruct=rv->type.s;
                Symbol *sMember=findSymbol(&sStruct->members,tkName->text);
                if(!sMember)
                    err("struct %s does not have a member %s exprPostfix",sStruct->name,tkName->text);
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
                if(rv->type.nElements<0)err("only an array can be indexed exprPostfix %d", rv->type.nElements);
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
                err("right bracket missing from postfix expression ");
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
            printf("expr unary\n");
            return index;
        }
        
        if((strcmp(enum_values[index->code], "SUB")==0) ||
            (strcmp(enum_values[index->code], "NOT")==0))
        {
            tkop = index;
            index = index->next;
        }

        if(tkop->code==SUB){
            if(rv->type.nElements>=0) err("unary '-' cannot be applied to an array exprUnary");
            if(rv->type.typeBase==TB_STRUCT)
                err("unary '-' cannot be applied to a struct exprUnary");
        }
        else{  // NOT
            if(rv->type.typeBase==TB_STRUCT)err("'!' cannot be applied to a struct exprUnary");
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
            if(!rv->isCtVal)err("the array size is not a constant arrayDecl");
            if(rv->type.typeBase!=TB_INT)err("the array size is not an integer arrayDecl");
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

Token * typeName(Token *start)
{
    Token * index = start;
    Type *t;
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

    t->nElements=-1;
    
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
                    state = 2;
                }
            }break;
            case 2:
            {
                start = start -> next;
                Token *copy = start;
                start = arrayDecl(start, t);
                if(copy==start)
                    t->nElements=-1;
                else
                    t->nElements=0;

                addVar(tkName, t);
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
                    // start = start -> next;
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

//TODO 
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

            // my_cast(t,&rve->type);
            // rv->type=*t;
            // rv->isCtVal=rv->isLVal=0;

            return index;
        }

        if(strcmp(enum_values[index->code], "LPAR")==0)
        {
            index=index->next;
            index = typeName(index);
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

    int flag=0;

    index=exprCast(index, rv);
    while(1)
    {
        if(flag)
            index=exprCast(index, rve);
        if(index!=start)
        {
            if((strcmp(enum_values[index->code], "MUL")==0)||
                (strcmp(enum_values[index->code], "DIV")==0))
            {
                flag=1;
                tkop = index;
                index=index->next;
            }
            else
            {
                if(flag==1)
                {
                    printf("%d %d\n", rv->type.nElements, rve->type.nElements);
                    printf("%d %d\n", rv->type.typeBase, rve->type.typeBase);
                    if(rv->type.nElements>-1||rve->type.nElements>-1)
                        err("an array cannot be multiplied or divided exprMul");
                    if(rv->type.typeBase==TB_STRUCT||rve->type.typeBase==TB_STRUCT)
                        err("a structure cannot be multiplied or divided exprMul");

                    rv->type=getArithType(&rv->type,&rve->type);
                    rv->isCtVal=rv->isLVal=0;

                }

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

    int flag=0;
    index=exprMul(index, rv);
    while(1)
    {
        if(flag)
            index=exprMul(index, rve);
        if(index!=start)
        {
            if((strcmp(enum_values[index->code], "ADD")==0)||
                (strcmp(enum_values[index->code], "SUB")==0))
            {
                tkop=index;
                index=index->next;
                flag=1;
            }
            else
            {
                if(flag)
                {
                    if(rv->type.nElements>-1||rve->type.nElements>-1)
                        err("an array cannot be added or subtracted exprAdd");
                    if(rv->type.typeBase==TB_STRUCT||rve->type.typeBase==TB_STRUCT)
                        err("a structure cannot be added or subtracted exprAdd");
                    //TODO de decomentat
                    rv->type=getArithType(&rv->type,&rve->type);
                    rv->isCtVal=rv->isLVal=0;        
                }
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

    int flag=0;
    index=exprAdd(index, rv);
    while(1)
    {
        if(flag)
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
                flag=1;
            }
            else
            {
                if(flag)
                {
                    printf("%d %d", rv->type.nElements, rve->type.nElements);
                    if(rv->type.nElements>-1||rve->type.nElements>-1)
                        err("an array cannot be compared exprRel");
                    if(rv->type.typeBase==TB_STRUCT||rve->type.typeBase==TB_STRUCT)
                        err("a structure cannot be compared exprRel");
                    rv->type=createType(TB_INT,-1);
                    rv->isCtVal=rv->isLVal=0;
                }

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
    int flag=0;
    index=exprRel(index, rv);
    while(1)
    {
        if(flag)
        index=exprRel(index, rve);
        if(index!=start)
        {
            if((strcmp(enum_values[index->code], "EQUAL")==0)||
                (strcmp(enum_values[index->code], "NOTEQ")==0))
            {
                tkop=index;
                index=index->next;
                flag=1;
            }
            else
            {
                if(flag)
                {
                    if(rv->type.typeBase==TB_STRUCT||rve->type.typeBase==TB_STRUCT)
                        err("a structure cannot be compared exprEq");
                    rv->type=createType(TB_INT,-1);
                    rv->isCtVal=rv->isLVal=0;
                }

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
    int flag=0;
    index=exprEq(index, rv);
    while(1)
    {
        index=exprEq(index, rve);
        if(index!=start)
        {
            if(strcmp(enum_values[index->code], "AND")==0)
            {
                tkop=index;
                index=index->next;
                flag=1;
            }
            else
            {
                if(flag)
                {
                    if(rv->type.typeBase==TB_STRUCT||rve->type.typeBase==TB_STRUCT)
                        err("a structure cannot be logically tested exprAnd");
                    rv->type=createType(TB_INT,-1);
                    rv->isCtVal=rv->isLVal=0;
                }

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
    int flag=0;
    index=exprAnd(index, rv);
    while(1)
    {
        index=exprAnd(index, rve);
        if(index!=start)
        {
            if(strcmp(enum_values[index->code], "OR")==0)
            {
                tkop=index;
                index=index->next;
                flag=1;
            }
            else
            {
                if(flag)
                {
                    if(rv->type.typeBase==TB_STRUCT||rve->type.typeBase==TB_STRUCT)
                        err("a structure cannot be logically tested exprOr");
                    rv->type=createType(TB_INT,-1);
                    rv->isCtVal=rv->isLVal=0;
                }

                printf("expr or\n");
                return index;
            }
        }
        else
            return start;
    }
}

//TODO 
Token *exprAssign(Token *start, RetVal* rv)
{
    Token *index = start, *tkop;
    RetVal *rve;
    SAFEALLOC(rve, RetVal)
    SAFEALLOC(tkop, Token)
    int flag=0;
    Token *copy = index;
    index=exprOr(index, rv);
    if(copy!=index && strcmp(enum_values[index->code], "ASSIGN")!=0)
    {
        printf("expr assign");
        return index;
    }
    index=start;
    while(1)
    {
        if(flag)
        {
            copy=index;
            index=exprOr(index, rve);
        }
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
            {
                flag=1;
                index=index->next;
            }
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

Symbol *addExtFunc(const char *name,Type type)
{
    Symbol *s=addSymbol(&symbols,name,CLS_FUNC);
    s->type=type;
    initSymbols(&s->args);
    return s;
}
Symbol *addFuncArg(Symbol *func,const char *name,Type type)
{
    Symbol *a=addSymbol(&func->args,name,CLS_VAR);
    a->type=type;
    return a;
}

void addExtraFunctions()
{
    Symbol *s=addExtFunc("put_s",createType(TB_VOID,-1));
    addFuncArg(s,"s",createType(TB_CHAR,0));

    s=addExtFunc("get_s",createType(TB_VOID,-1));
    addFuncArg(s,"s",createType(TB_CHAR,0));

    s=addExtFunc("put_i",createType(TB_VOID,-1));
    addFuncArg(s,"i",createType(TB_INT,-1));

    s=addExtFunc("get_s",createType(TB_INT,-1));
}

int syntactical_analysis(Token *tokens)
{
    addExtraFunctions();

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