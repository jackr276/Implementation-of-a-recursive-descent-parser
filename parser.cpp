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
		//Check for the compound statement, make sure that there actually is a BEGIN
		l = Parser::GetNextToken(in, line);
		if (l != BEGIN){
			ParseError(line, "Syntactic Error in Declaration Block.");
			ParseError(line, "Incorrect Declaration Section");
			return false;
		}

		//If we  have BEGIN, consume it and call CompoundStmt
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
	l = Parser::GetNextToken(in,line);

	//If we find the optional ASSOP, process it
	if (l == ASSOP){
		bool status = Expr(in, line);
		if (!status) {
			ParseError(line, "Invalid expression following assignment operator.");
			return false;
		}

	//If its unrecognized throw and error
	} else if (l == ERR){
		ParseError(line, "Unrecognized input pattern.");
		cout << "(" << l.GetLexeme() << ")";
		return false;

	//If we get here, l was not the optional ASSOP or ERR, push token back and return
	} else {
		Parser::PushBackToken(l);
	}

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
	bool status;
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
		status = SimpleStmt(in, line);

		if(!status){
			ParseError(line, "Incorrect Simple Statement.");
			return false;
		}

		return status;
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
	bool status;
	LexItem strd = Parser::GetNextToken(in, line);

	switch (strd.GetToken()){
		case IF:
			status = IfStmt(in, line);
			if (!status) {
				ParseError(line, "Bad structured statement.");
				return false;
			}
			return true;
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
	LexItem lookAhead;
	//If we got here we already have consumed a BEGIN
	bool status = Stmt(in, line);



	while(status) {
		l = Parser::GetNextToken(in, line);
		if (l != SEMICOL && l != END){
			ParseError(line, "Missing Semicolon in Compound statement.");
			return false;
		}

		status = Stmt(in, line);
	}


	if (l == ERR) {
		ParseError(line, "Unrecognized Input Pattern");
		//print out the unrecognized input
		cout << "(" << l.GetLexeme() << ")" << endl;
		return false;
	}


	if (l != END) {
		line++;
		ParseError(line, "Missing END in compound statement.");
		return false;
	}

	//If we make it to this point, we had valid expressions and saw END, so return true
	return true;
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
			Parser::PushBackToken(smpl);
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
bool WriteLnStmt(istream& in, int& line){
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

	//if expression is not valid, throw an error
	if(!status){
		ParseError(line, "Invalid expression in IF statement.");
		return false;
	}

	//if we get here, then we had a valid expression. Next token must be THEN
	l = Parser::GetNextToken(in, line);

	//If its unknown, throw error
	if (l == ERR ){
		ParseError(line, "Unrecognized Input Pattern");
		cout << "(" << l.GetToken() << ")" << endl;
		return false;
	}

	//If its not a THEN, we have an error
	if (l != THEN) {
		ParseError(line, "Missing THEN in IF statement.");
		return false;
	}

	//If we get here, we so far have IF expr THEN, check for a valid stmt
	status = Stmt(in, line);

	//If stmt is bad, throw error
	if(!status){
		ParseError(line, "Invalid statement in IF statement.");
		return false;
	}

	//at this point, we can see ELSE optionally, so check for it
	l = Parser::GetNextToken(in, line);

	//If we don't see ELSE then we're done, push token back and return
	if (l != ELSE) {
		Parser::PushBackToken(l);
		return status;
	}

	//If we get here, l was consumed and we should see a valid stmt
	status = Stmt(in, line);

	//If its invalid, throw an error
	if(!status){
		ParseError(line, "Invalid statement after ELSE in IF statement");
		return false;
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
				ParseError(line, "Bad Expression in Assignment Statement");
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

	//We should never get here, added to remove compile warnings
	return false;
}


/*
* Any expression in our grammar could optionally be a list of expressions
* Example: writeln(str, ' to cs 280 course')
* ExprList:= Expr {,Expr}
*/
bool ExprList(istream& in, int& line){
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


//Expr ::= LogOrExpr ::= LogAndExpr { OR LogAndExpr }
//So essentially: Expr ::= LogANDExpr { OR LogAndExpr}
bool Expr(istream& in, int& line){
	bool status = false;
	LexItem l;
	
	//Once we get here, first thing to do is call LogAndExpr
	status = LogANDExpr(in, line);

	//If expression is bad, return error
	if (!status){
		//ParseError(line, "Incorrect Expression");
		return false;
	}

	//Once we're here, we can either have nothing or one or more OR's followed by more LogAndExpr
	//Get the next token
	l = Parser::GetNextToken(in, line);

	//While we have an OR, keep processing LogAndExpr's
	while (l == OR){
		status = LogANDExpr(in, line);

		//If expression is bad, return error
		if (!status){
			//ParseError(line, "Incorrect Expression");
			return false;
		}

		//refresh the value of l
		l = Parser::GetNextToken(in, line);
	}

	// if we have an ERR token, throw error
	if (l == ERR) {
		ParseError(line, "Unrecognized input pattern.");
		cout << "(" << l.GetLexeme() << ")";
		return false;
	}

	//Once we get here, l was not an OR, so put it back and we're done
	Parser::PushBackToken(l);

	return status;
}


// LogAndExpr ::= RelExpr {AND RelExpr }
bool LogANDExpr(istream& in, int& line){
	bool status = false;
	LexItem l;

	//Once we get here, the first thing we should do is check for a relational expression
	status = RelExpr(in, line);

	//If we have a bad relational expression, return error
	if (!status){
		//ParseError(line, "Syntactic error in relational expression.");
		return false;
	}

	//Good first expression, check to see if we have an AND token
	l = Parser::GetNextToken(in, line);

	//So long as we keep seeing AND, keep processing tokens
	while (l == AND){
		//Check the next relational expression
		status = RelExpr(in, line);
		
		//If its a bad expression, throw an error and stop
		if (!status){
			//ParseError(line, "Incorrect relational expression.");
			return false;
		}

		//Refresh the value of l
		l = Parser::GetNextToken(in, line);
	}

	//If we got here, we know l wasn't AND, check if it is ERR
	if (l == ERR) {
		ParseError(line, "Unrecognized input pattern.");
		cout << "(" << l.GetLexeme() << ")";
		return false;
	}
	
	//If we get here, l wasn't AND or an ERR, so push it back to the stream
	Parser::PushBackToken(l);

	return status;
}


// RelExpr ::= SimpleExpr [ ( = | < | > ) SimpleExpr ]
bool RelExpr(istream& in, int& line){
	bool status;
	LexItem l;

	//We should first see a valid SimpleExpr
	status = SimpleExpr(in, line);

	if (!status) {
		ParseError(line, "Invalid Relational Expression");
		return false;
	}

	//If we get here we can optionally see =, < or > once
	l = Parser::GetNextToken(in, line);

	//If it is these, check for the validity of the simpleExpr
	if (l == EQ || l == GTHAN || l == LTHAN) {
		status = SimpleExpr(in, line);
		if(!status) {
			ParseError(line, "Invalid Relational Expression.");
			return false;
		}

		//If it is valid, we're done. Simply return status.
		return status;
	}

	//If lexeme is unknown, throw error
	if (l == ERR) {
		ParseError(line, "Unrecognized input pattern.");
		cout << "(" << l.GetLexeme() << ")";
		return false;
	}

	//If we didn't have =, < or > or ERR, push token back and return status
	Parser::PushBackToken(l);
	return status;
}


//SimpleExpr :: Term { ( + | - ) Term }
bool SimpleExpr(istream& in, int& line){
	bool status;
	LexItem l;

	//We should see a valid term first
	status = Term(in, line);

	//Throw error if invalid
	if(!status){
		//ParseError(line, "Invalid term in expression");
		return false;
	}

	//once we're here, we can see 0 or many + and -
	l = Parser::GetNextToken(in, line);

	//So long as we have plus or minus, we keep processing
	while (l == PLUS || l == MINUS) {
		status = Term(in, line);

		//If we have a bad term, throw error
		if(!status) {
			//ParseError(line, "Invalid term in expression.");
			return false;
		}

		//Refresh l
		l = Parser::GetNextToken(in, line);
	}

	//If lexeme is unknown, throw error
	if (l == ERR) {
		ParseError(line, "Unrecognized input pattern.");
		cout << "(" << l.GetLexeme() << ")";
		return false;
	}

	//Once we're here, we know l wasn't + or -, so we're done
	//Push l back and return status
	Parser::PushBackToken(l);
	return status;
}


//Term ::= SFactor { ( * | / | DIV | MOD ) SFactor }
bool Term(istream& in, int& line){
	bool status;
	LexItem l;

	//We must first see a valid Sfactor
	status = SFactor(in, line);

	//If SFactor is bad, no point in continuing
	if (!status) {
		//ParseError(line, "Bad SFactor in term.");
		return false;
	}

	//We can now see one or more *, /, DIV, or MODs followed by sfactors
	l = Parser::GetNextToken(in, line);

	//If we have any of these, process the next sfactor
	while (l == MULT || l == DIV || l == IDIV || l == MOD) {
		status = SFactor(in, line);

		//If SFactor is bad, no point in continuing
		if (!status) {
			ParseError(line, "Missing operand after operator.");
			return false;
		}

		//Refresh l
		l = Parser::GetNextToken(in, line);
	}

	//If we get here, we've had valid sfactors and l is no longer *. /, MOD or DIV
	//make sure l isn't an ERR
	if (l == ERR) {
		ParseError(line, "Unrecognized input pattern.");
		cout << "(" << l.GetLexeme() << ")" << endl;
		return false;
	}

	//If no error, push token back and return status
	Parser::PushBackToken(l);
	return status;
}


// SFactor can have an optional sign in front of it
// SFactor ::= [( - | + | NOT )] Factor
bool SFactor(istream& in, int& line){
	
	//Get the token for processing
	LexItem l = Parser::GetNextToken(in, line); 

	//Plus is a "1" in factor
	if (l == PLUS){
		return Factor(in, line, 1);
	}

	//Negative is a "2" in factor
	if (l == MINUS) {
		return Factor(in, line, 2);
	}

	//NOT is a "3" in factor
	if (l == NOT) {
		return Factor(in, line, 3);
	}

	//If l is not +, - or NOT, push token back and let factor handle it
	Parser::PushBackToken(l);
	//0 means we found no plus, minus or NOT
	return Factor(in, line, 0);
}


//Factor must be a predeclared identifier or a constant, or an optional expr in parenthesis
//Sign is 0 if no sign, 1 if positive(+), 2 if negative(-), 3 if NOT
//Factor ::= IDENT | ICONST | RCONST | SCONST | BCONST | (Expr)
bool Factor(istream& in, int& line, int sign){
	bool status;
	//get and check our first token
	LexItem l = Parser::GetNextToken(in, line);

	//If the token is an error, no bother in further processing
	if (l == ERR){
		ParseError(line, "Unrecognized input pattern.");
		cout << "(" << l.GetLexeme() << ")";
		return false;
	}

	//Check IDENT
	if (l == IDENT){
		//Idents should not have a sign at all
		if (sign != 0){
			ParseError(line, "Illegal use of a sign before an identifier.");
			return false;
		}
		
		//If we get here, ident was fine, push back and let Var handle it
		Parser::PushBackToken(l);
		return Var(in, line);
	}

	//Check SCONST
	if (l == SCONST){
		//SCONSTS should also have no sign
		if (sign != 0){
			ParseError(line, "Illegal use of a sign before a string constant.");
			return false;
		}
		//if we pass this condition then its true
		return true;
	}

	//Check RCONST and ICONST
	if (l == ICONST || l == RCONST){
		//Reals and ints can have +/- sign, or no sign, just not "NOT"
		if(sign == 3){
			ParseError(line, "Illegal use of NOT operator before integer or real constant.");
			return false;
		}

		return true;
	}

	//Check BCONST
	if (l == BCONST){
		//Booleans can have the NOT or no operator, but nothing else
		if (sign == 1 || sign == 2){
			ParseError(line, "Illegal use of +/- sign before boolean constant.");
			return false;
		}

		return true;
	}

	//If we see an lparen, we have an expr
	if (l == LPAREN){
		//Evaluate the internal expression
		status = Expr(in, line);

		if(!status){
			ParseError(line, "Invalid Expression.");
			return false;
		}

		//Ensure that there is a closing rparen
		l = Parser::GetNextToken(in, line);
		if (l != RPAREN){
			ParseError(line, "Missing Right Parenthesis");
			return false;
		}
	}

	return status;
}


// A simple wrapper that allows access to the number of syntax errors
int ErrCount(){
    return error_count;
}
