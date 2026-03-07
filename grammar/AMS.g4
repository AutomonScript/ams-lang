grammar AMS;

program : watchSection eventSection EOF ;

watchSection : 'WATCH:' watchItem* ;
watchItem    : ID ID '=' 'OPEN' '(' STRING ')' ;

eventSection : 'EVENTS:' eventDef* ;
eventDef     : 'EVENT' ID '{' statement* '}' ;

statement    : 'CONSOLE' '(' STRING ')' ';' ;

ID     : [a-zA-Z_][a-zA-Z0-9_]* ;
STRING : '"' (~["\r\n])* '"' ;
WS     : [ \t\r\n]+ -> skip ;