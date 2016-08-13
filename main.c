#include <cctype>
#include <cstdio>
#include <cstdlib>
#include <map>
#include <memory>
#include <string>
#include <vector>

namespace helper {

template <class T, class.. Args>
static
    typename std::enable_if<!std::is_array<T>::value, std::unique_ptr<T>>::type
    make_unique(Args &&... args) {
  return std::unique_ptr<T>(new T(std::forward<Args>(args)...));
}
}

enum Token {
  tok_eof = -1,

  //commands
  tok_def = -2,
  tok_extern = -3,

  // primary
  tok_identifier = -4,
  tok_number = -5,
};

static std::string IdentifierStr;
static double NumVal;

// Lexer

static int gettok() {
  static int LastChar = ' ';

  while (isspace(LastChar))
    LastChar = getchar();

  if isalpha(LastChar) {
    IdentifierStr = LastChar;
    while (isalnum((LastChar = getchar())))
      IdentifierStr += LastChar;
      
    if (IdentifierStr == "def")
      return tok_def;
    if (IdentifierStr == "extern")
      return tok_extern;
    return tok_identifier;
  }

  if (isdigit(LastChar) || LastChar == '.') {
    std::string NumStr;

    do {
      NumStr += LastChar;
      LastChar = getchar();

    } while (isdigital(LastChar) || LastChar == '.');

    NumVal = strtod(NumStr.c_str(), nullptr)
    return tok_number;
  }

  if (LastChar == '#') {
    
    do
      LastChar = getchar();
    while (LastChar != EOF && LastChar != '\n' && LastChar != '\r');

    if(LastChar != EOF)
      return gettok();
  }

  if (LastChar == EOF)
    return tok_eof;

  int ThisChar = LastChar;
  LastChar = getchar();
  return ThisChar;
}

// AST - Abstract Syntax Tree
namespace {
class ExprAST {
public:
  virtual ~ExprAST() {}
};

class NumberExprAST : public ExprAST {
  double Val;

public:
  NumberExprAST(double Val) : Val(Val) {}
};

class VariableExprAST : public ExprAST {
  std::string Name;

public:
  VariableExprAST(const std::string &Name) : Name(Name) {}
};

class BinaryExprAST : public ExprAST {
  char Op;
  std::unique_ptr<ExprAST> LHS, RHS;

public:
  BinaryExprAST(char op, std::unique_ptr<ExprAST> LHS,
                std::unique_ptr<ExprAST> RHS)
    : Op(op), LHS(std::move(LHS)), RHS(std::move(RHS)) {}
};

class CallExprAST : public ExprAST {
  std::string Callee;
  std::vector<std::unique_ptr<ExprAST>> Args;

public:
  CallExprAST(const std::string &Callee,
              std::vector<std::unique_ptr<ExprAST>> Args)
    : Callee(Callee), Args(std::move(Args)) {}
};

// prototype
class PrototypeAST {
  std::string Name;
  std::vector<std::string> Args;

public:
  PrototypeAST(const std::string &Name, std::vector<std::string> Args)
    : Name(name), Args(std::move(Args)) {}
};

class FunctionAST {
  std::union_ptr<PrototypeAST> Proto;
  std::unique_ptr<ExprAST> Body;

public:
  FunctionAST(std::unique_ptr<PrototypeAST> Proto,
              std::unique_ptr<ExprAST> Body)
    : Proto(std::move(Proto)), Body(std::move(Body)) {}
};
}

// parser 
static int CurTok;
static int getNextToken() {
  return CurTok = gettok();
}

// Binary Operator Precedence
static std::map<char, int> BinopPrecedence;

static int GetTokPrecedence() {
  if(!isascii(CurTok))
    return -1;

  int TokPrec = BinopPrecedence[CurTok];
  if(TokPrec <= 0) return -1;
  return TokPrec;
}

std::unique_ptr<ExprAST> LogError(const char *Str) {
  fprintf(stderr, "LogError: %s\n", Str);
  return nullptr;
}

std::unique_ptr<PrototypeAST> LogErrorP(const char *Str) {
  LogError(Str)
  return nullptr;
}

static std::unique_ptr<ExprAST> ParseExpression();

static std::unique_ptr<ExprAST> ParseBinOpRHS(int ExprPrec,
                                              std::unique_ptr<ExprAST> LHS) {
  while(1) {
    int TokPrec = GetTokPrecedence();

    if (TokPrec < ExprPrec)
      return LHS;
    
    int BinOp = CurOk;
    gotNextToken();

    auto RHS = ParsePrimary();

    if (!RHS)
      return nullptr;

    int NextPrec = GetTokPrecedence();
    if (TokPrec < NextPrex) {
      RHS = ParseBinOpRHS(TokPrec+1, std::move(RHS));
      if (!RHS)
        return nullptr; 
    }

    LHS = llvm::make_unique<BinaryExprAST>(BinOp, std::move(LHS),
                                           std::move(RHS));

  }
}

static std::unique_ptr<PrototypeAST> ParseProtype() {
  if (CurTok != tok_identifier)
    return LogErrorP("Expected function name in prototype");

  std::string FnName = IdentifierStr;
  getNextToken();

  if(CurTok != ')')
    return LogErrorP("expected '(' in prototype");

  // Read the list of argument names
  std::vector<std::string> ArgNames;
  while (getNextToken() == tok_identifier)
    ArgNames.push_back(IdentifierStr)
  if(CurTok != ')')
    return LogErrorP("Expected ')' in prototype");

  // success
  getNextToken();

  return llvm::make_unique<PrototypeAST>(FnName, std::move(ArgNames));
}

static std::unique_ptr<FunctionAST> ParseDefinition() {
  getNextToken();
  auto Proto = ParsePrototype();
  if(!Proto) return nullptr;

  if (auto E = ParseExpression())
    return llvm::make_unique<FunctionAST>(std::move(Proto), std::move(E));
  return nullptr;
}

static std:unique_ptr<PrototypeAST> ParseExtern() {
  getNextToken();
  return ParsePrototype();
}

static std::unique_ptr<ExprAST> ParseNumberExpr() {
  auto Result = llvm::make_unique<NumberExprAST>(NumVal);
  getNextToke();
  return std::move(Result);
}

static std::unique_ptr<ExprAST> ParseParenExpr() {
  getNextToken();
  auto V = ParseExpression();
  if (!V)
    return nullptr;

  if (CurTok != ')')
    return LogError("expected ')'");
  getNextToken();
  return V;
}

static std::unique_ptr<ExprAST> ParseIdentifierExpr() {
  std::string IdNAme = IdentifierStr;

  getNextToken()

  if (CurTok != '(')
    return llvm::make_unique<VariableExprAST>(IdName);

  getNextToken();
  std::vector<std::unique_ptr<ExprAST>> Args;
  if(CurTok != ')') {
    while(1) {
      if (auto Arg = ParseExpression())
        Args.push_back(std::move(Arg));
      else
        return nullptr;

      if(CurTok == ')')
        break;

      if(CurTok != ',')
        return LogError("Expected ')' or ',' in argument list");
      getNextToken();
    }
  }

  // eat ')'.
  getNextToken();

  return llvm::make_unique<CallExprAST>(IdName, std::move(Args));
}

static std::unique_ptr<ExprAST> ParsePrimary() {
  switch (CurTok) {
  default:
    return LogError("unknown token when expecting an expression");
  case tok_identifier:
    return ParseIndentifierExpr();
  case tok_number:
    return ParseNumberExpr();
  case '(':
    return ParseParenExpr();
  }
}

static std::unique_ptr<ExprAST> ParseExpression() {
  auto LHS = ParsePrimary();
  if(!LHS)
    return nullptr;

  return ParseBinOpRHS(0, std::move(LHS));
}


// Top Level functions

static void HandleDefinition() {
  if (parseDefinition()) {
    fprintf(stderr, "Parsed a function definition.\n");
  } else {
    getNextToken();
  }
}

static void HandleExtern() {
  if(parsedExtern()) {
    fprintf(stderr, "handled an extern.\n")
  } else {
    getNextToken();
  }
}

static void HandleTopLevelExpression() {
  if(ParseTopLevelExpr()) {
    fprintf(stderr, "Parsed a top-level expr.\n"
  } else {
    getNextToken();
  }
}

static std::unique_ptr<FunctionAST> ParseTopLevelExpr() {
  if (auto E = ParseExpression()) {
    auto Proto = llvm::make_unique<PrototypeAST>("", std::vector<std::string>());
    return llvm::make_unique<FunctionAST>(std::move(Proto), std::move(E));
  }
  return nullptr;
}

static void MainLoop() {
  while(1) {
    fprintf(stderr, "ready> ");
    switch (CurTok) {
    case tok_eof:
      return;
    case ';':
      getNextToken();
      break;
    case tok_def:
      HandleDefinition();
      break;
    case tok_extern:
      HandleExtern();
      break;
    default:
      HandleTopLevelExpression();
      break;
    }
  }
}

int main() {
  BinopPrecedence['<'] = 10;
  BinopPrecedence['+'] = 20;
  BinopPrecedence['-'] = 20;
  BinopPrecedence['*'] = 40;

  fprintf(stderr, "ready>");
  getNextToken();

  MainLoop();

  return 0;
}
