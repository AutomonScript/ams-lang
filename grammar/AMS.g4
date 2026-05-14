//######################## AutomonScript Grammer #############################

                                grammar AMS;

//############################### LEXER ######################################

//------------------------------ KEYWORDS ------------------------------------
// PROGRAM SECTIONS
GLOBAL  : 'GLOBAL';
SOURCES : 'SOURCES'; 
SOURCE  : 'SOURCE';
EVENTS  : 'EVENTS';
EVENT   : 'EVENT';
OBSERVERS : 'OBSERVERS';
OBSERVER  : 'OBSERVER';
FUNCTIONS : 'FUNCTIONS';
FUNCTION  : 'FUNCTION';

// GLOBAL SECTION
IMPORT : 'IMPORT';
MERGE  : 'MERGE';

// DATA TYPES
INT    : 'INT';
FLOAT  : 'FLOAT';
STRING : 'STRING';
BOOL   : 'BOOL';
VOID   : 'VOID';

// Conditional Statements
IF : 'IF';
ELSE_IF : 'ELSE'[ \t\r]+'IF';
ELSE : 'ELSE';

// Runtime System Keywords
TRACK       : 'TRACK';
CHECK       : 'CHECK';
EVERY       : 'EVERY';
AT_KW       : 'AT';
CONTINUOUSLY: 'CONTINUOUSLY';
ON          : 'ON';
OBSERVS     : 'OBSERVS';
UNSHARE     : 'UNSHARE';
SIGNAL      : 'SIGNAL';
//----------------------------------------------------------------------------
LBRACE  : '{';
RBRACE  : '}';
LPAREN  : '(';
RPAREN  : ')';
EQUAL   : '=';
SEMICOL : ';';
COL     : ':';
COMMA   : ',';
DOT     : '.';
NL      : '\n';

//------------------------------ OPERATORS -----------------------------------
// ARITHMETIC 
PLUS  : '+' | 'ADD';
MINUS : '-' | 'SUBTRACT';
MULT  : '*' | 'MULTIPLY';
DIV   : '/' | 'DIVIDE';
MOD   : '%' | 'REMAINDER';
POW   : '^' | 'POWER';

// RELATIONAL
EQ    : '==' | 'EQUALS';
NEQ   : '!=' | 'NOT' [ \t\r]+ 'EQUALS';
GT    : '>'  | 'GREATER' [ \t\r]+ 'THAN';
LT    : '<'  | 'LESS' [ \t\r]+ 'THAN';
GTE   : '>=' | 'GREATER' [ \t\r]+ 'EQUAL';
LTE   : '<=' | 'LESS' [ \t\r]+ 'EQUAL';

// LOGICAL
AND   : '&' | 'AND';
OR    : '|' | 'OR';
NOT   : '!' | 'NOT';
//----------------------------------------------------------------------------
WS       : [ \t\r]+     -> skip;
SLCOMMENT: '#' ~[\r\n]*      -> skip;
MLCOMMENT: '#''#'.*?'#''#' -> skip;
//----------------------------------------------------------------------------
STRING_L  : '"' (~["\r\n])* '"';
FLOAT_L   : [0-9]+ '.' [0-9]+;
INT_L     : [0-9]+;
TRUE    : 'TRUE';
FALSE   : 'FALSE';
// Time Unit Tokens (uppercase to match CaseCaptilizeInputStream)
MS_UNIT   : 'MS';
SEC_UNIT  : 'SEC';
MIN_UNIT  : 'MIN';
HOUR_UNIT : 'HOUR';

ID        : [A-Z_][A-Z0-9_]*;
//----------------------------------------------------------------------------

//############################### PARSER #####################################
// Support Grammer Rules
eos : SEMICOL?NL+ | EOF ; // END OF STATEMENT 

// Data Types 
dataType   : INT | FLOAT | STRING | BOOL | VOID ;
//----------------------------------------------------------------------------
// Program Sections 
program 
    : eos?
      (globalSection eos?)? 
      (sourcesSection eos?)? 
      (eventsSection eos?)? 
      (observersSection eos?)?
      (functionsSection eos?)? 
      EOF 
    ;
//----------------------------------------------------------------------------
// Common statements allowed in every program section 
statement
    : (variableDeclaration
      | assignment
      | functionCall
      ) eos
      | conditionalStatements eos?
    ;

variableDeclaration : dataType ID (EQUAL expression)? ;
sourceVariableDeclaration : (TRACK)? dataType ID (EQUAL expression)? ;
eventVariableDeclaration : (UNSHARE)? dataType ID (EQUAL expression)? ;
assignment          : ID EQUAL expression ;

conditionalStatements: IF LPAREN? expression RPAREN? NL* conditionalBlock 
                        (NL* ELSE_IF LPAREN? expression RPAREN? NL* conditionalBlock )* 
                        (NL* ELSE NL* conditionalBlock )? 
                        ;

conditionalBlock: LBRACE eos? statement* RBRACE 
                | eos? statement* SEMICOL
                | statement 
                ;


arguments : expression (COMMA expression)* ;
//----------------------------------------------------------------------------
// EXPRESSIONS 

expression
    : LPAREN expression RPAREN                         
    | op=(NOT | MINUS) expression
    | <assoc=right> expression op=POW expression 
    | expression op=(MULT | DIV | MOD) expression    
    | expression op=(PLUS | MINUS) expression  
    | expression op=(GT | LT | GTE | LTE) expression
    | expression op=(EQ | NEQ) expression
    | expression op=AND expression
    | expression op=OR expression
    | functionCall
    | dataAccess
    | ID                                      
    | STRING_L | INT_L | FLOAT_L | TRUE | FALSE
    ;

dataAccess : (SOURCE | EVENT | ID) DOT ID ;

//----------------------------------------------------------------------------
// Global Section 
globalSection : GLOBAL COL eos globalItem* ;

globalItem : importStatement 
           | mergeStatement 
           | statement
           ;

importStatement : IMPORT STRING_L eos;
mergeStatement  : MERGE STRING_L eos;

//----------------------------------------------------------------------------
// Time and Scheduling Rules
timeStatement   : EVERY INT_L timeUnit
                | AT_KW TIME_LITERAL
                | CONTINUOUSLY
                ;

timeUnit        : MS_UNIT | SEC_UNIT | MIN_UNIT | HOUR_UNIT ;

TIME_LITERAL    : [0-2][0-9] ':' [0-5][0-9] ;
//----------------------------------------------------------------------------
// Sources Section Structure 
sourcesSection : SOURCES COL eos sourceDefinition* ;
sourceDefinition : SOURCE ID sourceScheduleStatement LBRACE eos sourceItem* RBRACE eos
                 | SOURCE ID sourceScheduleStatement  eos sourceItem* SEMICOL eos;
sourceItem : sourceVariableDeclaration eos
           | assignment eos
           | functionCall eos
           | conditionalStatements eos?
           | SIGNAL expression? eos?
           ;

sourceScheduleStatement : (CHECK timeStatement)? ;
//----------------------------------------------------------------------------
// Events Section Structure
//----------------------------------------------------------------------------
// Events Section Structure
eventsSection : EVENTS COL eos eventDefinition* ;
eventDefinition : EVENT ID eventScheduleStatement LBRACE eos eventItem* RBRACE eos
                | EVENT ID eventScheduleStatement eos eventItem* SEMICOL eos;
eventItem : eventVariableDeclaration eos
          | assignment eos
          | functionCall eos
          | conditionalStatements eos?
          | SIGNAL expression? eos?
          ;

eventScheduleStatement : ON ID (SIGNAL expression)? ;
//----------------------------------------------------------------------------
// Observer Section Structure
observersSection : OBSERVERS COL eos observerDefinition* ;
observerDefinition : OBSERVER ID observerScheduleStatement LBRACE eos observerItem* RBRACE eos
                   | OBSERVER ID observerScheduleStatement  eos observerItem* SEMICOL eos;
observerItem : statement
             ;

observerScheduleStatement : OBSERVS ID ;
//----------------------------------------------------------------------------
// Functions Section Structure
functionsSection : FUNCTIONS COL eos functionDefinition* ;
functionDefinition : FUNCTION ID LPAREN parameters? RPAREN LBRACE eos functionItem* RBRACE eos
                   | FUNCTION ID LPAREN parameters? RPAREN eos functionItem* SEMICOL eos;
functionItem:  statement
            ;

parameters: parameter (COMMA parameter)* ;
parameter  : dataType ID ;
//----------------------------------------------------------------------------
functionCall: ID LPAREN arguments? RPAREN ;
//----------------------------------------------------------------------------