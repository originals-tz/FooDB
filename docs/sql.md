# FooDB SQL BNF

## Term

```
<identifier> ::= <alphabetic...>
<alphabetic> := a..zA..Z
<asterisk> ::= *
<comma> ::= ,
<equals operator> ::= =
<not equals operator> ::= !=
<less than operator> ::= <
<greater than operator> ::= >
<less than or equals operator> := <=
<greater than or equals operator> := >=
```

## Squery

```
<squery> ::= <query expression>
<query expression> ::= <query specification>
<query specification> ::= SELECT <select list> <table expression>

<select list> ::= <asterisk> | <select sublist> [ { <comma> <select sublist> }... ]
<select sublist> ::= <identifier>

<table expression>    ::=   <from clause> [ <where clause> ]
<from clause>    ::=   FROM <table reference list>
<table reference list> ::= <table reference> [<comma> <table reference>]
<table reference> := <identifier>

<where clause>    ::=   WHERE <search condition>
<search condition>    ::=   <boolean value expression>
<boolean value expression>    ::=
        <boolean term>
    |   <boolean value expression> OR <boolean term>
<boolean term>    ::=
        <boolean factor>
    |   <boolean term> AND <boolean factor>
<boolean factor>    ::=   [ NOT ] <boolean test>
<boolean test>  ::= <identifier> <comp op> <value>
<comp op>    ::=
       <equals operator>
    |  <not equals operator>
    |  <less than operator>
    |  <greater than operator>
    |  <less than or equals operator>
    |  <greater than or equals operator>
```
