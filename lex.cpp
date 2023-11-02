/**
 * Author: Jack Robbins
 * 10/25/2023
 * Programming assignment 1
*/

#include "lex.h"
#include <map>
#include <algorithm>

using namespace std;

// we will need a map of all possible token types
static map<Token, string> tokenMap = {
    //identifiers
    {IDENT, "IDENT"},

    // Booleans
    {TRUE, "TRUE"},
    {FALSE, "FALSE"},

    //keywords - these will also need their own map for id_or_kw
    {IF, "IF"}, 
    {ELSE, "ELSE"},
    {WRITELN, "WRITELN"},
    {WRITE, "WRITE"},
    {INTEGER, "INTEGER"},
    {REAL, "REAL"},
	{BOOLEAN, "BOOLEAN"},
    {STRING, "STRING"},
    {BEGIN, "BEGIN"},
    //{END, "END"},
    {VAR, "VAR"},
    {THEN, "THEN"},
    {PROGRAM, "PROGRAM"},


    //int, real or string const
    {ICONST, "ICONST"},
    {RCONST, "RCONST"},
    {SCONST, "SCONST"},
    {BCONST, "BCONST"},

    //operators
    {PLUS, "PLUS"},
    {MINUS, "MINUS"},
    {MULT, "MULT"},
    {DIV, "DIV"},
    {IDIV, "IDIV"},
    {MOD, "MOD"},
    {ASSOP, "ASSOP"},
    {EQ, "EQ"},
    {GTHAN, "GTHAN"},
    {LTHAN, "LTHAN"},
    {AND, "AND"},
    {OR, "OR"},
    {NOT, "NOT"},

    //Delimiters
    {COMMA, "COMMA"},
    {SEMICOL, "SEMICOL"},
    {LPAREN, "LPAREN"},
    {RPAREN, "RPAREN"},
    {DOT, "DOT"},
    {COLON, "COLON"},

    //these will make the program terminate
    {ERR, "ERR"},
    {DONE, "DONE"}
}; 


//Additionally, we need a separate map of all keywords for id_or_kw
// This map has the keys and values reversed, so that we can search for tokens by lexeme
static map<string, Token> keywordMap = {
    {"IF", IF}, 
    {"ELSE", ELSE},
    {"WRITELN", WRITELN},
    {"WRITE", WRITE},
    {"INTEGER", INTEGER},
    {"REAL", REAL},
	{"BOOLEAN", BOOLEAN},
    {"STRING", STRING},
    {"BEGIN", BEGIN},
    //{"END", END},
    {"VAR", VAR},
    {"THEN", THEN},
    {"PROGRAM", PROGRAM},
    {"DIV", IDIV},
    {"MOD", MOD},
    {"AND", AND},
    {"OR", OR},
    {"NOT", NOT},
    {"TRUE", BCONST},
    {"FALSE", BCONST}
};

/*
Break -> break out of the case statement/loop completely
Continue -> simply skip to the next iteration
*/

LexItem getNextToken(istream& in, int& linenumber){
    //Enum for all of the states a token could be in
    enum tokState{START, INID, ININT, INREAL, INSTRING, INCOMMENT}
    //We naturally begin in the start state
    lexstate = START;
    Token t = ERR;
    string lexeme = "";
    char ch;
    bool inString = false;

    while(in.get(ch)){
        switch(lexstate){
            case START: // we are at the beginning of a new lexeme
                //at a newline character, simply increment line number
                if (ch == '\n'){
                    linenumber++;
                    //go to next character
                    continue;
                }

                //ignoring whitespace, so just continue
                if(isspace(ch)){
                    //go to next character
                    continue;
                }

                //Anything below here should have the character be in the lexeme(we ignore whitespace and tabs)
                lexeme += ch;

                //if it's a digit then we're in an ININT state by default
                if (isdigit(ch)){
                    lexstate = ININT;
                    continue;

                //identifiers can begin with letters, _ or $, so seeing this would put us in the INID state
                //Note - keywords also start with characters, so we'll have to check using is_id_or_kw in the INID state
                } else if (isalpha(ch) || ch == '_' || ch == '$'){
                    lexstate = INID;
                    continue;

                // Strings must start with a single ', so seeing this puts us in the INSTRING state
                } else if (ch == '\''){
                    inString = true;
                    lexstate = INSTRING;
                    //go to next character
                    continue;
                
                // Comments start with a '{', so if we find one of these we start being in a comment
                } else if (ch == '{'){
                    lexstate = INCOMMENT;
                    // reset the lexeme
                    lexeme = "";
                    continue;
                } 
                // If we've gotten here, we've got a token of some kind
                else {
                    //By default, if we don't change our tok in the switch statement, we'll have an error
                    //from above, token t = ERR
                    switch(ch){
                        case '<':
                            t = LTHAN;
                            //break out of the switch once any case is true
                            break;
                        case '>':
                            t = GTHAN;
                            break;
                        case '+':
                            t = PLUS;
                            break;
                        case '-':
                            t = MINUS;
                            break;
                        case '*':
                            t = MULT;
                            break;
                        case '/':
                            t = DIV;
                            break;
                        case '=':
                            t = EQ;
                            break;
                        // With this one, we could have COLON or ASSOP, have to check
                        case ':':
                            t = COLON;
                            if (in.peek() == '='){
                                // if we see an equal sign, we should consume the token right in here, as to not repeat
                                in.get(ch);
                                lexeme += ch;
                                t = ASSOP;
                                
                            }
                            break;
                        case ';':
                            t = SEMICOL;
                            break;
                        case '.':
                            t = DOT;
                            break;
                        case '(':
                            t = LPAREN;
                            break;
                        case ')':
                            t = RPAREN;
                            break;
                        case ',':
                            t = COMMA;
                            break;
                    }
                    // After this switch-case, return the token we got, or an error for an unrecognized token
                    return LexItem(t, lexeme, linenumber);
                }

            
            case INSTRING:
                // Strings are never allowed to have newlines, so if we see one its an immediate error
                if (ch == '\n'){
                    return LexItem(ERR, lexeme, linenumber);
                }
                //if we see this character, the string is over, reset the state and return LexItem;
                if (ch == '\''){
                    lexstate = START;
                    inString = false;
                    return LexItem(SCONST, lexeme.substr(1, lexeme.size()-1), linenumber);
                
                } else {
                    lexeme += ch;
                }
                //get out of the switch statement and onto the next character
                break;
            
            
            case ININT:
                //if the character is an int, just add it to the lexeme, no change of state
                if(isdigit(ch)){
                    lexeme += ch;
                //the first part of a real could be an int, so if we see a dot we should switch states
                } else if (ch == '.'){
                    lexeme += ch;
                    lexstate = INREAL;
                } else {
                    //ch could be a letter, or a space, but to be safe let's put it back so it can be rechecked
                    in.putback(ch);
                    //We're done with the int, so start a new lexeme
                    lexstate = START;
                    //Return a lexItem of type ICONST
                    return LexItem(ICONST, lexeme, linenumber);
                }
                break;


            case INREAL:
                // if the character is a digit simply add it to lexeme
                if (isdigit(ch)){
                    lexeme += ch;
                
                //if we're in this state, there was already a dot, so finding another one would be erronious
                } else if (ch == '.'){
                    lexeme += ch;
                    return LexItem(ERR, lexeme, linenumber);
                // we either have a space or a different character
                } else {
                    // needs to be consumed again
                    in.putback(ch);
                    //New lexeme
                    lexstate = START;
                    //return a lexitem of type RCONST
                    return LexItem(RCONST, lexeme, linenumber);
                }
                break;


            case INID:
                //ID's are allowed to have letters, numbers, underscores and $'s, so these are all fine 
                if (isalpha(ch)||isdigit(ch) || ch == '_' || ch == '$'){
                    lexeme += ch;
                //we've found the end of the id
                } else {
                    //Put the character back to be reprocessed
                    in.putback(ch);
                    //State is at start again
                    lexstate = START;
                    //Use the id_or_kw helper function to appropriately return either the IDENT token or the keyword 
                    return id_or_kw(lexeme, linenumber);
                }
                break;

            case INCOMMENT:
            //Multiline commonts are allowed, so if we find a newline character, just incremement line number
            if (ch == '\n'){
                linenumber++;
            }

            //end of comment if we find this
            if (ch == '}'){
                //new lexeme
                lexstate = START;
            }
            continue;
        }
    }

    //If we're at the end of the file, return the DONE token
    if(in.eof() && inString){
        return LexItem(ERR, lexeme, linenumber);

    //If we get to eof, we're done so return the DONE token
    }else if(in.eof()){
        return LexItem(DONE, "", linenumber);
    }

    //If we reach here then some error must have happened
    return LexItem();
}



/*
* The lexItem function takes in a reference to a string and a linenumber, and checks if given lexeme is in the keywordMap
* @returns: a lexItem with either an IDENT token or the keyword token, if one was found
*/
LexItem id_or_kw(const string& lexeme, int linenum) {
    //If the word isn't a keyword, we'll have Ident as our default token
    Token token = IDENT;

    string lexemeUpper = lexeme;
    //Since everything in our map is uppercase, we'll need to make our lexeme uppercase
    transform(lexemeUpper.begin(), lexemeUpper.end(), lexemeUpper.begin(), ::toupper);

    //See if we can find this keyword as a key in the specially made keywordMap
    auto i = keywordMap.find(lexemeUpper); 

    //if we found the lexeme as a key, we know the token should be the corresponding value("i->second")
    if (i != keywordMap.end()){
        token = i -> second;
    }

    return LexItem(token, lexeme, linenum);
}


/*
* The overloaded << operator prints out a lexitem according to what its token is. If the token is an error, it will print an 
* error message. If a token is a
*/
ostream& operator<<(ostream& out, const LexItem& tok){
    Token t = tok.GetToken();
    // If there's an error token, print the appropriate message
    if (t == ERR){
        out << "Error in line " << tok.GetLinenum() + 1 << ": Unrecognized Lexeme {" << tok.GetLexeme() << "}";   
        return out; 
    }
    // if t is one of these, we want to print the lexeme with it as well
    if (t == IDENT || t == BCONST || t == ICONST || t == RCONST || t == SCONST){ 
        out << tokenMap[t] << ": \"" << tok.GetLexeme() << "\""; 
    // otherwise, just print out the string version of the token
    } else {
        out << tokenMap[t];
    }

    return out;
}
