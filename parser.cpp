/**
* Author: Jack Robbins
* 11/13/2023
* Programming Assignment 2 - implementation of a recursive descent parser for a pascal-like language. It uses the previously built tokenizer lex.cpp to get tokens
* from a program, and then applies grammar rules to check for the validity of expressions
*/

#include "parser.h"
#include <iostream>

// defVar keeps track of all variables that have been defined in the program thus far
map<string, bool> defVar;

map<string, Token> SymTable;

namespace Parser {
	bool pushed_back = false;
	LexItem	pushed_token;

	static LexItem GetNextToken(istream& in, int& line) {
		if( pushed_back ) {
			pushed_back = false;
			return pushed_token;
		}
		return getNextToken(in, line);
	}

	static void PushBackToken(LexItem & t) {
		if( pushed_back ) {
			abort();
		}
		pushed_back = true;
		pushed_token = t;	
	}

}

static int error_count = 0;

int ErrCount()
{
    return error_count;
}

void ParseError(int line, string msg)
{
	++error_count;
	cout << line << ": " << msg << endl;
}

/** 
 * This is the top of our parse tree, everything that we take in is either a statement or expression of some kind
* Grammar Rules
* Stmt ::= SimpleStmt | StructuredStmtStmt
* SimpleStmt ::= AssignStmt | WriteLnStmt | WriteStmt
* StructuredStmt ::= IfStmt | CompoundStmt
*/
bool Stmt(istream& in, int& line) {

	// Get the next lexItem from the instream and analyze it
	LexItem l = Parser::GetNextToken(in, line);

	// Check if we have a structured statement
	if (l == BEGIN || l == IF){
		//Put token back to be reprocessed
		Parser::PushBackToken(l);
		return StructuredStmt(in, line);
	}

	// Check to see if we have a simple statement
	if (l == VAR || l == WRITE || l == WRITELN){
		//Put token back to be reprocessed
		Parser::PushBackToken(l);
		return SimpleStmt(in, line);
	}

	//We didn't find anything so push the token back
	Parser::PushBackToken(l);
	//Stmt was not successful if we got here
	return false;
}

/**
* stmt will call SimpleStmt if appropriate according to our grammar rules
* SimpleStmt ::= AssignStmt | WriteLnStmt | WriteStmt
*/
bool SimpleStmt(istream& in, int& line){
	LexItem smpl = Parser::GetNextToken(in, line);

	switch (smpl.GetToken()){
		//Assignments start with VARs
		case VAR:
			return AssignStmt(in, line);

		case WRITELN:
			return WriteLnStmt(in, line);

		case WRITE:
			return WriteStmt(in, line);
		
		//We won't ever get here, added for compile safety on Vocareum
		default:
			return false;
	}
}

/**
* stmt will call StructuredStmt if appropriate according to our grammar rules
* StructuredStmt ::= IfStmt | CompoundStmt
*/

bool StructuredStmt(istream& in, int& line){
	LexItem strd = Parser::GetNextToken(in, line);

	switch (strd.GetToken()){
		case IF:
			return IfStmt(in, line);
		
		//Compound statements begin with in
		case BEGIN:
			return CompoundStmt(in, line);

		default:
			//we won't ever get here, added to remove compile warnings
			return false;
	}
}



bool Prog(istream& in, int& line){
	return false;
}

/**
 * Write statements must have open and closing parenthesis with an ExprList inside
 * WriteStmt ::= write (ExprList)
*/
bool WriteStmt(istream& in, int& line){
	//Get the token after the word "write" and check if its an lparen
	LexItem t = Parser::GetNextToken(in, line);

	//No left parenthesis is an error, create error and exit
	if (t != LPAREN) {
		ParseError(line, "Missing Left Parenthesis");
		return false;
	}

	//Generate the ExprList recursively
	bool expr = ExprList(in, line);

	//If no ExprList was gotten create a different error
	if (!expr){
		ParseError(line, "Missing expression list for Write statement");
		return false;
	}

	//Check for a right parenthesis
	t = Parser::GetNextToken(in, line);
	
	if (t != RPAREN) {
		ParseError(line, "Missing right Parenthesis");
		return false;
	}

	return expr;

}


//WriteLnStmt ::= writeln (ExprList) 
bool WriteLnStmt(istream& in, int& line) {
	LexItem t;
	//cout << "in WriteStmt" << endl;
	
	t = Parser::GetNextToken(in, line);
	if( t != LPAREN ) {
		
		ParseError(line, "Missing Left Parenthesis");
		return false;
	}
	
	bool ex = ExprList(in, line);
	
	if( !ex ) {
		ParseError(line, "Missing expression list for WriteLn statement");
		return false;
	}
	
	t = Parser::GetNextToken(in, line);
	if(t != RPAREN ) {
		
		ParseError(line, "Missing Right Parenthesis");
		return false;
	}
	//Evaluate: print out the list of expressions values

	return ex;
}//End of WriteLnStmt


/*
* Any expression in our grammar could optionally be a list of expressions
* Example: writeln(str, ' to cs 280 course')
* ExprList:= Expr {,Expr}
*/
bool ExprList(istream& in, int& line) {
	bool status = false;
	//Get the first Expr, as their is always a starting expression
	status = Expr(in, line);

	//If we don't find an expression, return an error
	if(!status){
		ParseError(line, "Missing Expression");
		return false;
	}
	
	//Let's check if we have a comma
	LexItem tok = Parser::GetNextToken(in, line);
	
	//If we do, recursively call ExprList again
	if (tok == COMMA) {
		status = ExprList(in, line);
	}

	//If there's an error, we have an unrecognized input pattern
	else if(tok.GetToken() == ERR){
		ParseError(line, "Unrecognized Input Pattern");
		//print out the unrecognized input
		cout << "(" << tok.GetLexeme() << ")" << endl;
		return false;
	}

	// If there's no comma, we've reached the end. Return the token and return true
	else {
		Parser::PushBackToken(tok);
		return true;
	}

	//We should never get here, added to avoid warnings
	return status;
}


bool Expr(istream& in, int& line){
	return false;
}


