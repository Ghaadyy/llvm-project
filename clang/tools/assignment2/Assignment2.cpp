
#include "clang/AST/AST.h"
#include "clang/AST/ASTConsumer.h"
#include "clang/AST/RecursiveASTVisitor.h"
#include "clang/ASTMatchers/ASTMatchers.h"
#include "clang/Frontend/ASTConsumers.h"
#include "clang/Frontend/CompilerInstance.h"
#include "clang/Frontend/FrontendActions.h"
#include "clang/Rewrite/Core/Rewriter.h"
#include "clang/Tooling/CommonOptionsParser.h"
#include "clang/Tooling/Tooling.h"

#include <sstream>
#include <map>
#include <stack>

using namespace clang::driver;
using namespace clang::tooling;
using namespace clang;

static llvm::cl::OptionCategory Assignment2Category("Assignment 2 Command Line Options");
static llvm::cl::opt<std::string>
    outFileName("o",
        llvm::cl::desc("Output file name"),
        llvm::cl::init("codegen.ir"),
        llvm::cl::cat(Assignment2Category));


class Assignment2CodeGenerator : public ConstStmtVisitor<Assignment2CodeGenerator> {

    public:

        Assignment2CodeGenerator() : EC(), outFile_(outFileName, EC, llvm::sys::fs::OF_Text) {
            if (EC) {
                llvm::errs() << "Error: Could not write to file (" << EC.message() << ")\n";
                exit(0);
            }
        }

        void VisitFunctionDecl(FunctionDecl* funcDecl) {

            if(Stmt* body = funcDecl->getBody()) {

                // Generate code for the function signature

                // TODO
                outFile_  << "define " << funcDecl->getReturnType() << " @" << funcDecl->getName().str() << "(";

                if (funcDecl->getNumParams() > 0) {
                  outFile_ << (*funcDecl->param_begin())->getType() << " %" << (*funcDecl->param_begin())->getName().str();
                  if (funcDecl->getNumParams() > 1) {
                    for (auto p = funcDecl->param_begin() + 1; p != funcDecl->param_end(); ++p) {
                      const auto param = *p;
                      outFile_ << ", " << param->getType() << " %" << param->getName().str();
                    }
                  }
                }

                outFile_ << ") {\n\n";

                // Generate code for the function body
                // Do not forget to store parameters to the stack upon entry

                // TODO

                for (const auto param : funcDecl->parameters()) {
                  // %r0 = alloca int
                  // store int %x, int* %r0
                  const auto reg = createReg(param);
                  outFile_ << "%r" << reg << " = alloca " << param->getType() << "\n";
                  outFile_ << "store " << param->getType() << " %" << param->getNameAsString() << ", " << param->getType() << "* %r" << reg << "\n";
                }

                Visit(body);

                outFile_ << "\n}\n\n";
            }

        }

        void VisitCompoundStmt(const CompoundStmt* stmt) {

            // Generate code for compund statements (sequence of statements in curly braces)

            // TODO
            for (const auto s : stmt->children()) {
              Visit(s);
            }
        }

        void VisitDeclStmt(const DeclStmt* stmt) {

            for(auto decl = stmt->decl_begin(); decl != stmt->decl_end(); ++decl) {
                if(const VarDecl* vdecl = dyn_cast<VarDecl>(*decl)) {

                    // Generate code for variable declaration(s)

                    // TODO
                    const auto reg = createReg(vdecl);
                    outFile_ << "%r" << reg << " = alloca " << vdecl->getType() << "\n";
                    if (vdecl->hasInit()) {
                      Visit(vdecl->getInit());
                      outFile_ << "store " << vdecl->getType() << " %r" << getReg(vdecl->getInit()) << ", " << vdecl->getType() << "* %r" << reg << "\n";
                    }
                }
            }

        }

        void VisitDeclRefExpr(const DeclRefExpr* E) {

            // You only need to support references to variables
            const VarDecl* vdecl = dyn_cast<VarDecl>(E->getDecl());
            assert(vdecl != NULL && "Unsupported reference");

            // Generate code for variable references

            // TODO
            setReg(E, getReg(vdecl));
        }

        void VisitIntegerLiteral(const IntegerLiteral *literal) {

            // Generate code for integer constants

            // TODO
            const auto reg = createReg(literal);
            outFile_ << "%r" << reg << " = " << "seti int " << literal->getValue() << "\n";
        }

        void VisitCharacterLiteral(const CharacterLiteral *literal) {

            // Generate code for character constants

            // TODO
            const auto reg = createReg(literal);
            outFile_ << "%r" << reg << " = " << "seti char " << literal->getValue() << "\n";
        }

        void VisitFloatingLiteral(const FloatingLiteral *literal) {

            // Generate code for float constants

            // TODO
            const auto reg = createReg(literal);
            outFile_ << "%r" << reg << " = " << "seti float " << literal->getValue() << "\n";
        }

        void VisitImplicitCastExpr(const ImplicitCastExpr* cast) {

            if(cast->getCastKind() != CK_LValueToRValue) {

                // You do not need to support implicit type casts, just lvalue to rvalue casts
                const Expr* sub = cast->getSubExpr();
                Visit(sub);
                setReg(cast, getReg(sub));

            } else {

                // Generate code for lvalue to rvalue casts

                // TODO
                const Expr* sub = cast->getSubExpr();
                Visit(sub);
                const auto reg = createReg(cast);
                outFile_ << "%r" << reg << " = load " << sub->getType() << ", " << sub->getType() << "* %r" << getReg(sub) << "\n";

            }

        }

        void VisitBinaryOperator(const BinaryOperator *BO) {

            BinaryOperator::Opcode op = BO->getOpcode();

            // You do not need to support pointer-to-member binary operators (.*, ->*)
            assert(op != BO_PtrMemI && op != BO_PtrMemD && "Pointer-to-member operators not supported");

            if(op >= BO_Mul && op <= BO_Or) {

                // Generate code for arithmetic, logical, and relational operators
                // Hint: you can use BO->getOpcodeStr() to get the operator symbol

                // TODO
                Visit(BO->getLHS());
                Visit(BO->getRHS());
                const auto reg = createReg(BO);
                outFile_ << "%r" << reg  << " = " << BO->getLHS()->getType() << " %r" << getReg( BO->getLHS()) << " " << BO->getOpcodeStr() << " "
                                                  << BO->getRHS()->getType() << " %r" << getReg( BO->getRHS()) << "\n";

            } else if(op == BO_Assign) {

                // Generate code for the assignment operator

                // TODO
                const auto lhs = BO->getLHS();
                const auto rhs = BO->getRHS();
                // const auto reg = createReg(BO);

                Visit(lhs);
                Visit(rhs);

                // store int %r142, int* %r141
                outFile_ << "store " << lhs->getType() << " %r" << getReg(rhs) << ", " << lhs->getType() << "* %r" << getReg(lhs) << "\n";
                setReg(BO, getReg(lhs));

            } else if(op >= BO_MulAssign && op <= BO_OrAssign) {

                // Generate code for compound assignment operators
                // Hint: Options are BO_MulAssign (*=), BO_DivAssign (/=), BO_RemAssign (%=), BO_AddAssign (+=), BO_SubAssign (-=), 
                //                   BO_ShlAssign (<<=), BO_ShrAssign (>>=), BO_AndAssign (&=), BO_XorAssign (^=), BO_OrAssign  (|=)

                // TODO
                // %r33 = load int, int* %r28
                // %r34 = load int, int* %r29
                // %r35 = int %r34 + int %r33
                // store int %r35, int* %r29
                const auto lhs = BO->getLHS();
                const auto rhs = BO->getRHS();

                Visit(lhs);
                Visit(rhs);
                const auto r1 = createReg();
                outFile_ << "%r" << r1 << " = load " << lhs->getType() << ", " << lhs->getType() << "* %r" << getReg(lhs) << "\n";
                // outFile_ << "%r" << r2 << " = load " << rhs->getType() << ", " << rhs->getType() << "* %r" << getReg(rhs) << "\n";
                const auto opCode = BinaryOperator::getOpcodeStr(op);
                const auto reg = createReg();
                outFile_ << "%r" << reg << " = " << lhs->getType() << " %r" << r1 << " " << opCode.substr(0, opCode.size() - 1) << " " << rhs->getType() << " %r" << getReg(rhs) << "\n";
                outFile_ << "store " << rhs->getType() << " %r" << reg << ", " << lhs->getType() << "* %r" << getReg(lhs) << "\n";
                setReg(BO, getReg(lhs));

            } else if(op == BO_LAnd || op == BO_LOr) {

                // Generate code for short-circuiting Boolean expressions

                // TODO

                const unsigned int nextLabel = createLabel();
                const unsigned int exitLabel = createLabel();

                Visit(BO->getLHS());
                const auto r1 = getReg(BO->getLHS());
                outFile_ << "br %r" << r1 << ", label L" << (op == BO_LAnd ? nextLabel : exitLabel) << ", label L" << (op == BO_LAnd ? exitLabel : nextLabel) << "\n";
                outFile_ << "\nL" << nextLabel << ":\n";
                Visit(BO->getRHS());
                const auto r2 = getReg(BO->getRHS());
                outFile_ << "br label L" << exitLabel << "\n";
                outFile_ << "\nL" << exitLabel << ":\n";
                const auto reg = createReg(BO);
                outFile_ << "%r" << reg << " = phi(%r" << r1 << ", %r" << r2 << ")\n";

            } else {

                assert(0 && "Unsupported binary operator!");

            }

        }

        void VisitUnaryOperator(const UnaryOperator* UO) {

            UnaryOperator::Opcode op = UO->getOpcode();

            if(op >= UO_Plus && op <= UO_LNot) {

                // Generate code for unary arithmetic, logical, and Boolean operators
                // Hint: you can use UnaryOperator::getOpcodeStr(op) to get the operator symbol

                // TODO
                const auto sub = UO ->getSubExpr();
                Visit(sub);

                const auto reg = createReg(UO);
                const auto opCode = UnaryOperator::getOpcodeStr(op);

                // outFile_ << "%r" << r1 << " = load " << sub->getType() << ", " << sub->getType() << "* %r" << getReg(sub) << "\n";
                outFile_ << "%r" << reg << " = " << opCode << " " << sub->getType() << " %r" << getReg(sub) << "\n";

            } else if(op == UO_PostInc || op == UO_PostDec) {

                // Generate code for post-fix operators (e.g., x++, x--)

                // TODO
                const auto sub = UO ->getSubExpr();
                Visit(sub);

                const auto r1 = createReg(), r2 = createReg(), reg = createReg();
                const auto opCode = UnaryOperator::getOpcodeStr(op);

                outFile_ << "%r" << r1 << " = load " << sub->getType() << ", " << sub->getType() << "* %r" << getReg(sub) << "\n";
                outFile_ << "%r" << r2 << " = seti " << sub->getType() << " 1\n";
                outFile_ << "%r" << reg << " = " << sub->getType() << " %r" << r1 << " " << opCode[0] << " " << sub->getType() << " %r" << r2 << "\n";
                outFile_ << "store " << sub->getType() << " %r" << reg << ", " << sub->getType() << "* %r" << getReg(sub) << "\n";
                setReg(UO, r1);

            } else if(op == UO_PreInc || op == UO_PreDec) {

                // Generate code for pre-fix operators (e.g., ++x, --x)

                // TODO
                const auto sub = UO ->getSubExpr();
                Visit(sub);

                const auto r1 = createReg(), r2 = createReg(), r3 = createReg();
                const auto opCode = UnaryOperator::getOpcodeStr(op);

                outFile_ << "%r" << r1 << " = load " << sub->getType() << ", " << sub->getType() << "* %r" << getReg(sub) << "\n";
                outFile_ << "%r" << r2 << " = seti " << sub->getType() << " 1\n";
                outFile_ << "%r" << r3 << " = " << sub->getType() << " %r" << r1 << " " << opCode[0] << " " << sub->getType() << " %r" << r2 << "\n";
                outFile_ << "store " << sub->getType() << " %r" << r3 << ", " << sub->getType() << "* %r" << getReg(sub) << "\n";
                setReg(UO, getReg(sub));

            } else if(op == UO_AddrOf) {

                // Generate code for address-of operator (e.g., &x)

                // TODO
                Visit(UO->getSubExpr());
                setReg(UO, getReg(UO->getSubExpr()));

            } else if(op == UO_Deref) {

                // Generate code for dereference operator (e.g., *x)

                // TODO
                Visit(UO->getSubExpr());
                setReg(UO, getReg(UO->getSubExpr()));

            } else {

                assert(0 && "Unsupported unary operator!");

            }
        }

        void VisitCallExpr(const CallExpr* call) {

            // Generate code for call expressions

            // TODO

            for (const auto arg : call->arguments()) {
              Visit(arg);
            }

            const auto reg = createReg(call);
            outFile_ << "%r" << reg << " = call " << call->getDirectCallee()->getReturnType() << " @" << call->getDirectCallee()->getNameAsString() << "(";

            if (call->getNumArgs() > 0) {
              outFile_ << (*call->arg_begin())->getType() << " %r" << getReg(*call->arg_begin());
              if (call->getNumArgs() > 1) {
                for (auto a = call->arg_begin() + 1; a != call->arg_end(); ++a) {
                  const auto arg = *a;
                  outFile_ << ", " << arg->getType() << " %r" << getReg(arg);
                }
              }
            }

            outFile_ << ")\n";
        }

        void VisitReturnStmt(const ReturnStmt* Stmt) {

            // Generate code for return statements

            // TODO
            if (const Expr* val = Stmt->getRetValue()) {
              Visit(val);
              outFile_ << "ret " << val->getType() << " %r" << getReg(val) << "\n";
            } else {
              outFile_ << "ret void\n";
            }
        }

        void VisitParenExpr(const ParenExpr* expr) {

            // Generate code for expressions in parantheses

            // TODO
            Visit(expr->getSubExpr());
            setReg(expr, getReg(expr->getSubExpr()));
        }

        void VisitNullStmt(const NullStmt* stmt) {

            // Generate code for empty statement

            // TODO

        }

        void VisitIfStmt(const IfStmt* ifStmt) {

            // Generate code for if statement

            // TODO
            const unsigned int thenLabel = createLabel(), elseLabel = createLabel(), exitLabel = createLabel();

            Visit(ifStmt->getCond());
            outFile_ << "br %r" << getReg(ifStmt->getCond()) << ", label L" << thenLabel << ", label L" << (ifStmt->hasElseStorage() ? elseLabel : exitLabel) << "\n";
            outFile_ << "\nL" << thenLabel << ":\n";
            Visit(ifStmt->getThen());
            outFile_ << "br label L" << exitLabel << "\n";
            if (ifStmt->hasElseStorage()) {
              outFile_ << "\nL" << elseLabel << ":\n";
              Visit(ifStmt->getElse());
              outFile_ << "br label L" << exitLabel << "\n";
            }
            outFile_ << "\nL" << exitLabel << ":\n";
        }

        void VisitConditionalOperator(const ConditionalOperator* CO) {

            // Generate code for ternary operator

            // TODO
            const auto trueLabel = createLabel(), falseLabel = createLabel(), exitLabel = createLabel();
            Visit(CO->getCond());
            outFile_ << "br %r" << getReg(CO->getCond()) << ", label L" << trueLabel << ", label L" << falseLabel << "\n";
            outFile_ << "\nL" << trueLabel << ":\n";
            Visit(CO->getTrueExpr());
            outFile_ << "br label L" << exitLabel << "\n";
            outFile_ << "\nL" << falseLabel << ":\n";
            Visit(CO->getFalseExpr());
            outFile_ << "br label L" << exitLabel << "\n";
            outFile_ << "\nL" << exitLabel << ":\n";
            const auto reg = createReg(CO);
            outFile_ << "%r" << reg << " = phi(%r" << getReg(CO->getTrueExpr()) << ", %r" << getReg(CO->getFalseExpr()) << ")\n";
        }

        void VisitWhileStmt(const WhileStmt* whileStmt) {

            // Generate code for while loop

            // TODO
            const auto condLabel = createLabel(), bodyLabel = createLabel(), exitLabel = createLabel();
            pushBreakLabel(exitLabel);
            pushContinueLabel(condLabel);

            outFile_ << "\nL" << condLabel << ":\n";
            Visit(whileStmt->getCond());
            outFile_ << "br %r" << getReg(whileStmt->getCond()) << ", label L" << bodyLabel << ", label L" << exitLabel << "\n";
            outFile_ << "\nL" << bodyLabel << ":\n";
            Visit(whileStmt->getBody());
            outFile_ << "br label L" << condLabel << "\n";
            outFile_ << "\nL" << exitLabel << ":\n";

            popBreakLabel();
            popContinueLabel();
        }

        void VisitDoStmt(const DoStmt* doStmt) {

            // Generate code for do-while loop

            // TODO
            const auto bodyLabel = createLabel(), condLabel = createLabel(), exitLabel = createLabel();

            pushBreakLabel(exitLabel);
            pushContinueLabel(condLabel);

            outFile_ << "\nL" << bodyLabel << ":\n";
            Visit(doStmt->getBody());
            outFile_ << "br label L" << condLabel << "\n";
            outFile_ << "\nL" << condLabel << ":\n";
            Visit(doStmt->getCond());
            outFile_ << "br %r" << getReg(doStmt->getCond()) << ", label L" << bodyLabel << ", label L" << exitLabel << "\n";
            outFile_ << "\nL" << exitLabel << ":\n";

            popBreakLabel();
            popContinueLabel();
        }

        void VisitForStmt(const ForStmt* forStmt) {

            // Generate code for for loop

            // TODO
            const auto condLabel = createLabel(), bodyLabel = createLabel(), postIterLabel = createLabel(), exitLabel = createLabel();

            pushBreakLabel(exitLabel);
            pushContinueLabel(condLabel);

            Visit(forStmt->getInit());
            outFile_ << "br label L" << condLabel << "\n";
            outFile_ << "\nL" << condLabel << ":\n";
            Visit(forStmt->getCond());
            outFile_ << "br %r" << getReg(forStmt->getCond()) << ", label L" << bodyLabel << ", label L" << exitLabel << "\n";
            outFile_ << "\nL" << bodyLabel << ":\n";
            Visit(forStmt->getBody());
            outFile_ << "br label L" << postIterLabel << "\n";
            outFile_ << "\nL" << postIterLabel << ":\n";
            Visit(forStmt->getInc());
            outFile_ << "br label L" << condLabel << "\n";
            outFile_ << "\nL" << exitLabel << ":\n";

            popBreakLabel();
            popContinueLabel();
        }

        void VisitBreakStmt(const BreakStmt* B) {

            // Generate code for break statement

            // TODO
            const auto label = getBreakLabel();
            outFile_ << "br label L" << label << "\n";
        }

        void VisitContinueStmt(const ContinueStmt* C) {

            // Generate code for continue statement

            // TODO
            const auto label = getContinueLabel();
            outFile_ << "br label L" << label << "\n";
        }

        void VisitArraySubscriptExpr(const ArraySubscriptExpr* E) {
            // You do not need to support arrays
            assert(0 && "Arrays not supported");
        }

        void VisitSwitchStmt(const SwitchStmt* stmt) {
            // You do not need to support switch statements
            assert(0 && "Switch statement not supported");
        }

        void VisitCaseStmt(const CaseStmt* stmt) {
            // You do not need to support switch statements
            assert(0 && "Switch statement not supported");
        }

        void VisitDefaultStmt(const DefaultStmt* stmt) {
            // You do not need to support switch statements
            assert(0 && "Switch statement not supported");
        }

        void VisitStringLiteral(const StringLiteral *literal) {
            // You do not need to support string literals
            assert(0 && "String literals not supported");
        }

        void VisitMemberExpr(const MemberExpr* expr) {
            // You do not need to support complex objects
            assert(0 && "Complex objects not supported");
        }

        void VisitCompoundLiteralExpr(const CompoundLiteralExpr *E) {
            // You do not need to support complex objects
            assert(0 && "Complex objects not supported");
        }

        void VisitInitListExpr(const InitListExpr* E) {
            // You do not need to support complex objects
            assert(0 && "Complex objects not supported");
        }

        void VisitStmt(const Stmt* stmt) {
            // You do not need to support any other type of statement
            assert(0 && "Unsupported statement!");
        }

    private:

        std::error_code EC;
        llvm::raw_fd_ostream outFile_;

        // Used to generate unique virtual registers
        unsigned int regGenerator = 0;
        std::map<const VarDecl*,unsigned int> vdecl2reg_;
        std::map<const Expr*,unsigned int> expr2reg_;

        // Create a unique virtual register for a local variable
        unsigned int createReg(const VarDecl* vdecl) {
            assert(!vdecl2reg_.count(vdecl));
            vdecl2reg_[vdecl] = regGenerator++;
            return vdecl2reg_[vdecl];
        }

        // Create a unique virtual register for the temporary result of an expression evaluation
        unsigned int createReg(const Expr* expr) {
            assert(!expr2reg_.count(expr));
            expr2reg_[expr] = regGenerator++;
            return expr2reg_[expr];
        }

        // Create a unique virtual register for a temporary result that is not the direct result of an expression evaluation
        unsigned int createReg() {
            return regGenerator++;
        }

        // Get the unique virtual register for a local variable
        unsigned int getReg(const VarDecl* vdecl) {
            assert(vdecl2reg_.count(vdecl));
            return vdecl2reg_[vdecl];
        }

        // Get the unique virtual register for the temporary result of an expression evaluation
        unsigned int getReg(const Expr* expr) {
            assert(expr2reg_.count(expr));
            return expr2reg_[expr];
        }

        // Set the virtual register of an expression evaluation to some existing register
        void setReg(const Expr* expr, unsigned int reg) {
            assert(!expr2reg_.count(expr));
            expr2reg_[expr] = reg;
        }

        // Used to generate labels and track the destination of break and continue
        unsigned int labelGenerator = 0;
        std::stack<unsigned int> breakLabels_;
        std::stack<unsigned int> continueLabels_;

        // Create a unique label
        unsigned int createLabel() { return labelGenerator++; }

        // Manipulate the break destination stack
        void pushBreakLabel(unsigned int label) { breakLabels_.push(label); }
        void popBreakLabel() { breakLabels_.pop(); }
        unsigned int getBreakLabel() { return breakLabels_.top(); }

        // Manipulate the continue destination stack
        void pushContinueLabel(unsigned int label) { continueLabels_.push(label); }
        void popContinueLabel() { continueLabels_.pop(); }
        unsigned int getContinueLabel() { return continueLabels_.top(); }

};

class Assignment2Visitor : public RecursiveASTVisitor<Assignment2Visitor> {

    public:

        Assignment2Visitor() { }

        bool VisitFunctionDecl(FunctionDecl *funcDecl) {
            codeGenerator_.VisitFunctionDecl(funcDecl);
            return true;
        }

    private:

        Assignment2CodeGenerator codeGenerator_;

};

class Assignment2ASTConsumer : public ASTConsumer {

    public:

        Assignment2ASTConsumer() {}

        virtual void HandleTranslationUnit(ASTContext &Context) override {
            TranslationUnitDecl* TU = Context.getTranslationUnitDecl();
            Assignment2Visitor visitor;
            visitor.TraverseDecl(TU);
        }

};

class Assignment2Action : public ASTFrontendAction {

    public:

        Assignment2Action() {}

        std::unique_ptr<ASTConsumer> CreateASTConsumer(CompilerInstance &CI, StringRef file) override {
            return std::make_unique<Assignment2ASTConsumer>();
        }

};

int main(int argc, const char **argv) {
    auto ExpectedParser = tooling::CommonOptionsParser::create(argc, argv, Assignment2Category);
    if (!ExpectedParser) {
        llvm::errs() << ExpectedParser.takeError();
        return 1;
    }
    tooling::CommonOptionsParser &OptionsParser = ExpectedParser.get();
    ClangTool Tool(OptionsParser.getCompilations(), OptionsParser.getSourcePathList());
    return Tool.run(newFrontendActionFactory<Assignment2Action>().get());
}

