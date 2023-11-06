/**
* Author: Jack Robbins
* 11/13/2023
* Programming Assignment 2 - implementation of a recursive descent parser for a pascal-like language. It uses the previously built tokenizer lex.cpp to get tokens
* from a program, and then applies grammar rules to check for the validity of expressions
*/

#include "parser.h"
#include <iostream>
#include <set>

// defVar keeps track of all variables that have been defined in the program thus far
map<string, bool> defVar;
// SymTable keeps track of the type for all of our variables
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


//Initialize error count to be 0
static int error_count = 0;


//A simple error wrapper that incrememnts error count, and prints out the error
void ParseError(int line, string msg)
{
	++error_count;
	cout << line << ": " << msg << endl;
}


/**
 * To start, the program must use the keyword Program and give an identifier name.
 * It must then go into the Declaritive part followed by a compound statement
 * Prog ::= PROGRAM IDENT ; DeclPart CompoundStmt
*/
bool Prog(istream& in, int& line){
	bool status = false;

	//This should be the keyword "program"
	LexItem l = Parser::GetNextToken(in, line);

	//We're missing the required program keyword, throw an error and exit
	if (l != PROGRAM){
		ParseError(line, "Missing PROGRAM keyword.");
		return false;
	} else {
		//We have the program keyword, move on to more processing
		l = Parser::GetNextToken(in, line);

		//This token should be an IDENT if all is correct, if not we have an error
		if (l != IDENT){
			ParseError(line, "Missing Program name.");
			return false;
		}

		//If we're at this point we have PROGRAM IDENT, need a semicol
		l = Parser::GetNextToken(in, line);

		//If there's no semicolon, syntax error
		if (l != SEMICOL) {
			ParseError(line, "Syntax Error.");
			return false;
		}

		//By this point, we have checked up to PROGRAM IDENT ;
		//Check the DeclPart
		status = DeclPart(in, line);
		
		//If the declaration was bad, no point in continuing
		if (!status){
			ParseError(line, "Incorrect Declaration Section.");
			return false;
		}

		//Up to here we have gotten PROGRAM IDENT ; DeclPart
		//Check for the compound statement
		status = CompoundStmt(in, line);

		//If the compound statement was bad, return false
		if (!status){
			ParseError(line, "Incorrect Program Body.");
			return false;
		}

	}

	//There could also be some unrecognizable token here
	if (l == ERR) {
		ParseError(line, "Unrecognized input pattern.");
		cout << "(" << l.GetLexeme() << ")" << endl;
		return false; 
	}

	//If we reach here, status will be true and parsing will have been successful
	return status;
}


/**
 * The declarative part must start with the var keyword, followed by one or more colon separated declStmt's
 * DeclPart ::= VAR DeclStmt; { DeclStmt ; }
*/
bool DeclPart(istream& in, int& line){
	bool status = false;

	LexItem l = Parser::GetNextToken(in, line);
	//This first token should be VAR, if not throw an error
	if (l != VAR){
		ParseError(line, "Non-recognizable Declaration Part.");
		return false;
	}

	//Once we're here, we should be seeing DeclStmt's followed by SEMICOLs
	//There can be as many as we like, so use iteration

	//We will use this lexitem to look ahead
	LexItem lookAhead = Parser::GetNextToken(in, line);
	while(lookAhead == IDENT){
		//Once we know its an ident, put it back for processing by DeclStmt
		Parser::PushBackToken(lookAhead);
		
		//DeclStmt processing
		status = DeclStmt(in, line);
		
		//If its a bad DeclStmt, throw error
		if (!status) {
			ParseError(line, "Syntactic error in Declaration Block.");
			return false;
		}

		//We had a good DeclStmt, it has to be followed by a semicol
		l = Parser::GetNextToken(in, line);

		//If no semicolon, throw syntax error
		if (l != SEMICOL){
			//error right here
			ParseError(line, "Syntactic error in Declaration Block.");
			return false;
		}

		//Update lookahead, this will tell us if we have more declstmts
		lookAhead = Parser::GetNextToken(in, line);
	}

	//If we get here, lookAhead must not have been an IDENT. We need to put it back for processing by the CompoundStmt block
	Parser::PushBackToken(lookAhead);

	//If we get here, our DeclPart will have been successful
	return status;
}


/**
 * A delcaration statement can have one or more comma separated identifiers, followed by a valid type and an optional assignment
 * DeclStmt ::= IDENT {, IDENT } : Type [:= Expr]
*/
bool DeclStmt(istream& in, int& line){
	//All of the variables in a declstmt are going to have the same type, store in a set for type assignment
	set<string> tempSet;

	//Dummy lexItem to make the first iteration of the while loop run
	LexItem lookAhead = LexItem(COMMA, ",", 0);
	LexItem l;

	//We should see an IDENT first
	while (lookAhead == COMMA) {
		l = Parser::GetNextToken(in, line);
		//l must be an IDENT
		if (l != IDENT) {
			ParseError(line, "Non-indentifier declaration.");
			return false;
		}

		//If this variable is already in defVars, we have a redeclaration, throw error
		if (defVar.find(l.GetLexeme()) -> second){
			ParseError(line, "Variable Redefinition");
			ParseError(line, "Incorrect identifiers list in Declaration Statement.");
			return false;
		}

		//If we get here, it wasn't in defVars, so we should add it
		defVar.insert(pair<string, bool> (l.GetLexeme(), true));
		tempSet.insert(l.GetLexeme());

		lookAhead = Parser::GetNextToken(in, line);
	}
	
	//If we're out of the loop, we know it wasn't a comma
	//If there's an ident after this, then we know the user forgot to put a comma in between
	if (lookAhead == IDENT){
		ParseError(line, "Missing comma in declaration statement");
		//Having this would also make it a bad identifier list, so return this error as well
		ParseError(line, "Incorrect identifiers list in Declaration Statement.");
		return false;
	}

	//If its not a colon at this point, there's some syntax error here
	//Let caller handle
	if (lookAhead != COLON){
		return false;
	}


	//following this, we need to have a type for our variables
	//The next token should be a valid type
	l = Parser::GetNextToken(in, line);
	//allowed to be integer, boolean, real, string
	if (l == STRING || l == INTEGER || l == REAL || l == BOOLEAN){
		for(auto i : tempSet){
			SymTable.insert(pair<string, Token> (i, l.GetToken()));
		}
	} else {
		//Unrecognized type
		ParseError(line, "Incorrect Declaration Type.");
		return false;
	}

	//Once we get here, we have found 
	//DeclStmt ::= IDENT {, IDENT } : Type
	//After type, there is an optional ASSOP, so get the next token to check
	//l = Parser::GetNextToken(in,line);
	//TODO

	return true;
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

	// If l is uncrecognizable, no use in checking anything
	if (l == ERR){
		ParseError(line, "Unrecognized input pattern.");
		cout << "(" << l.GetLexeme() << ")";
		return false;
	}

	// Check if we have a structured statement
	if (l == BEGIN || l == IF){
		//Put token back to be reprocessed
		Parser::PushBackToken(l);
		return StructuredStmt(in, line);
	}

	// Check to see if we have a simple statement
	// Assignments start with IDENT
	if (l == IDENT || l == WRITE || l == WRITELN){
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


/**
 * Compound statements start with BEGIN and stop with END
 * CompoundStmt ::= BEGIN Stmt {; Stmt } END
*/
bool CompoundStmt(istream& in, int& line){
	LexItem l;
	//If we got here we already have consumed a BEGIN
	bool stmt = Stmt(in, line);
	//Analyze/consume the statement

	//If the first statement was valid, continue on
	if(stmt){
		//We can either have an END or a SEMICOL
		l = Parser::GetNextToken(in, line);

		//there can be as many semicols as the user likes, keep calling stmt for them
		while (l.GetToken() == SEMICOL){
			stmt= Stmt(in, line);
			//For each stmt we get, check for validity
			if(!stmt){
				//If this fails, then we have a bad statement
				ParseError(line, "Syntactic Error in the statement");
				return false;
			}

			//Refresh l
			l = Parser::GetNextToken(in, line);
		}


 	} 

	return false;
}


/**
* stmt will call SimpleStmt if appropriate according to our grammar rules
* SimpleStmt ::= AssignStmt | WriteLnStmt | WriteStmt
*/
bool SimpleStmt(istream& in, int& line){
	LexItem smpl = Parser::GetNextToken(in, line);

	switch (smpl.GetToken()){
		//Assignments start with identifiers
		case IDENT:
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


//FIXME needs documentation
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


// Processing all IF statements, 
// IfStmt ::= IF Expr THEN Stmt [ ELSE Stmt ]
bool IfStmt(istream& in, int& line){
	LexItem l;

	//Once this function is called, the IF token has been consumed already
	//We should see a valid expression at this point
	bool status = Expr(in, line);

	//if expression is not valid, return false
	if(!status){

	}
	return status;



}


/**
 * Assignment Statements take in a var, ASSOP and expression
 * AssignStmt ::= Var := Expr
*/
bool AssignStmt(istream& in, int& line){
	bool status = false;
	bool varStatus = false;
	LexItem l;

	//Check to see the status of the identifier that we have(was it already declared?)
	varStatus = Var(in, line);

	if (varStatus) {
		//Get the next token(should be assop)
		l = Parser::GetNextToken(in, line);

		if (l == ASSOP){
			//Analyze the expression after the assignment operator
			status = Expr(in, line);
			
			//If there's no expression, thats an error
			if (!status){
				ParseError(line, "Missing Expression in Assignment Statement");
				return false;
			}

		//Unrecognized token
		} else if (l == ERR){
			ParseError(line, "Unrecognized Input Pattern");
			//print out the unrecognized input
			cout << "(" << l.GetLexeme() << ")" << endl;
			return false;

		//If we get here there was no assignment operator
		} else {
			ParseError(line, "Missing Assignment Operator in AssignStmt");
			return false;
		}

	//If the variable wasn't correct, we essentially have no variable
	} else {
		ParseError(line, "Missing Left-Hand Side Variable in Assignment statement");
		return false;
	}
	return status;
}


// Check to see if the variable is valid and has previously been declared
// Var ::= IDENT
bool Var(istream& in, int& line){
	//get the token, check to see if var was declared
	LexItem l = Parser::GetNextToken(in, line);

	//If we can find the variable, return true
	if(defVar.find(l.GetLexeme())-> second){
		return true;
	//If lexeme is unrecognized, then give this error
	} else if (l == ERR) {
		ParseError(line, "Unrecognized Input Pattern");
		cout << "(" << l.GetToken() << ")" << endl;
		return false;
	//If we get here, we have a valid variable name that was just not declared. Show appropriate error
	} else {
		ParseError(line, "Undeclared Variable");
		return false;
	}
}



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

bool LogANDExpr(istream& in, int& line){
	return false;
}

bool RelExpr(istream& in, int& line){
	return false;
}

bool SimpleExpr(istream& in, int& line){
	return false;
}

bool Term(istream& in, int& line){
	return false;
}

bool SFactor(istream& in, int& line){
	return false;
}

bool Factor(istream& in, int& line, int sign){
	return false;
}


// A simple wrapper that allows access to the number of syntax errors
int ErrCount()
{
    return error_count;
}
