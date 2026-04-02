/*
    Assignment 2 prepared by:
    Antoine Karam (ack13@mail.aub.edu), ID: 202670543
    Ghady Youssef (ggy03@mail.aub.edu), ID: 202670367
*/

#include "clang/AST/AST.h"
#include "clang/AST/ASTConsumer.h"
#include "clang/AST/RecursiveASTVisitor.h"
#include "clang/Frontend/ASTConsumers.h"
#include "clang/Frontend/FrontendActions.h"
#include "clang/Frontend/CompilerInstance.h"
#include "clang/Tooling/CommonOptionsParser.h"
#include "clang/Tooling/Tooling.h"
#include "clang/Rewrite/Core/Rewriter.h"

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
                std::string returnType = funcDecl->getReturnType().getAsString();
                std::string funcName = funcDecl->getNameAsString();
                outFile_ << "\ndefine " << returnType << " @" << funcName << "(";

                for(unsigned i = 0; i < funcDecl->getNumParams(); i++) {
                    if(i > 0) outFile_ << ", ";
                    ParmVarDecl* param = funcDecl->getParamDecl(i);
                    std::string paramType = param->getType().getAsString();
                    std::string paramName = param->getNameAsString();
                    outFile_ << paramType << " %" << paramName;
                }

                outFile_ << ") {\n\n";

                // Generate code for the function body
                // Do not forget to store parameters to the stack upon entry

                for (const auto param : funcDecl->parameters()) {
                  const auto reg = createReg(param);
                  outFile_ << "%r" << reg << " = alloca " << param->getType() << "\n";
                  outFile_ << "store " << param->getType() << " %" << param->getNameAsString() << ", " << param->getType() << "* %r" << reg << "\n";
                }
                Visit(body);

                outFile_ << "\n}\n";
            }

        }

        void VisitCompoundStmt(const CompoundStmt* stmt) {

            // Generate code for compund statements (sequence of statements in curly braces)

            // TODO
            for(const auto& st: stmt->body()) {
                Visit(st);
            }

        }

        void VisitDeclStmt(const DeclStmt* stmt) {

            for(auto decl = stmt->decl_begin(); decl != stmt->decl_end(); ++decl) {
                if(const VarDecl* vdecl = dyn_cast<VarDecl>(*decl)) {

                    // Generate code for variable declaration(s)

                    // TODO
                    std::string vType = vdecl->getType().getAsString();
                    unsigned int reg = createReg(vdecl);
                    outFile_ << "%r" << reg << " = alloca " << vType << "\n";

                    if(const Expr* init = vdecl->getInit()) {
                        Visit(init);
                        unsigned int initReg = getReg(init);
                        outFile_ << "store " << vType << " %r" << initReg
                                 << ", " << vType << "* %r" << reg << "\n";
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
            int64_t value = literal->getValue().getSExtValue();
            unsigned int reg = createReg(literal);
            outFile_ << "%r" << reg << " = seti int " << value << "\n";

        }

        void VisitCharacterLiteral(const CharacterLiteral *literal) {

            // Generate code for character constants

            // TODO
            unsigned value = literal->getValue();
            unsigned int reg = createReg(literal);
            outFile_ << "%r" << reg << " = seti char " << value << "\n";

        }

        void VisitFloatingLiteral(const FloatingLiteral *literal) {

            // Generate code for float constants

            // TODO
            std::string type = literal->getType().getAsString();
            float value = literal->getValue().convertToFloat();
            unsigned int reg = createReg(literal);
            outFile_ << "%r" << reg << " = seti float " << value << "\n";

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
                const Expr* lhs = BO->getLHS();
                const Expr* rhs = BO->getRHS();
                Visit(lhs);
                Visit(rhs);

                unsigned int reg = createReg(BO);
                unsigned int lhsReg = getReg(lhs);
                unsigned rhsReg = getReg(rhs);

                llvm::StringRef symbol = BO->getOpcodeStr();

                outFile_ << "%r" << reg << " = "
                         << lhs->getType() << " %r" << lhsReg
                         << " " << symbol << " "
                         << rhs->getType() << " %r" << rhsReg << "\n";

            } else if(op == BO_Assign) {

                // Generate code for the assignment operator

                // TODO
                const Expr* lhs = BO->getLHS();
                const Expr* rhs = BO->getRHS();
                Visit(lhs);
                Visit(rhs);

                unsigned int lhsReg = getReg(lhs);
                unsigned int rhsReg = getReg(rhs);

                outFile_ << "store " << BO->getType() << " %r" << rhsReg << ", "
                         << BO->getType() << "* %r" << lhsReg << "\n";

                setReg(BO, lhsReg);

            } else if(op >= BO_MulAssign && op <= BO_OrAssign) {

                // Generate code for compound assignment operators
                // Hint: Options are BO_MulAssign (*=), BO_DivAssign (/=), BO_RemAssign (%=), BO_AddAssign (+=), BO_SubAssign (-=),
                //                   BO_ShlAssign (<<=), BO_ShrAssign (>>=), BO_AndAssign (&=), BO_XorAssign (^=), BO_OrAssign  (|=)

                // TODO
                const Expr* lhs = BO->getLHS();
                const Expr* rhs = BO->getRHS();
                Visit(lhs);
                Visit(rhs);

                unsigned int lhsReg = getReg(lhs);
                unsigned int rhsReg = getReg(rhs);

                std::string type = BO->getType().getAsString();

                unsigned int lTmp = createReg();
                outFile_ << "%r" << lTmp << " = load " << type
                         << ", " << type << "* %r" << lhsReg << "\n";

                BinaryOperator::Opcode simpleOp = BinaryOperator::getOpForCompoundAssignment(op);
                llvm::StringRef symbol = BinaryOperator::getOpcodeStr(simpleOp);

                unsigned int resultReg = createReg();
                outFile_ << "%r" << resultReg << " = "
                         << type << " %r" << lTmp
                         << " " << symbol << " "
                         << type << " %r" << rhsReg << "\n";

                outFile_ << "store " << type << " %r" << resultReg
                        << ", " << type << "* %r" << lhsReg << "\n";

                setReg(BO, lhsReg);

            } else if(op == BO_LAnd || op == BO_LOr) {

                // Generate code for short-circuiting Boolean expressions

                // TODO
                unsigned int nextLabel = createLabel();
                unsigned int exitLabel = createLabel();

                const Expr* lhs = BO->getLHS();
                Visit(lhs);
                unsigned int lhsReg = getReg(lhs);

                outFile_ << "br %r" << lhsReg << ", label L"
                         << (op == BO_LAnd ? nextLabel : exitLabel)
                         << ", label L" << (op == BO_LAnd ? exitLabel : nextLabel) << "\n";

                outFile_ << "\nL" << nextLabel << ":\n";
                const Expr* rhs = BO->getRHS();
                Visit(rhs);
                outFile_ << "br label L" << exitLabel << "\n";

                outFile_ << "\nL" << exitLabel << ":\n";
                unsigned int reg = createReg(BO);
                unsigned int rhsReg = getReg(rhs);
                outFile_ << "%r" << reg
                         << " = phi(%r" << lhsReg
                         << ", %r" << rhsReg << ")\n";

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
                const Expr* subExpr = UO->getSubExpr();
                Visit(subExpr);

                unsigned int reg = createReg(UO);
                unsigned int subExprReg = getReg(subExpr);

                llvm::StringRef symbol = UnaryOperator::getOpcodeStr(op);

                outFile_ << "%r" << reg << " = "
                         << symbol << " "
                         << subExpr->getType() << " %r" << subExprReg << "\n";

            } else if(op == UO_PostInc || op == UO_PostDec) {

                // Generate code for post-fix operators (e.g., x++, x--)

                // TODO
                const Expr* subExpr = UO->getSubExpr();
                Visit(subExpr);

                unsigned int subExprReg = getReg(subExpr);
                std::string type = UO->getType().getAsString();

                unsigned int reg = createReg(UO), consReg = createReg(), resultReg = createReg();

                char opStr = UnaryOperator::getOpcodeStr(op)[0];

                outFile_ << "%r" << reg << " = load " << type
                         << ", " << type << "* %r" << subExprReg << "\n";
                outFile_ << "%r" << consReg << " = seti " << type << " 1\n";
                outFile_ << "%r" << resultReg << " = "
                         << type << " %r" << reg
                         << " " << opStr << " "
                         << type << " %r" << consReg << "\n";
                outFile_ << "store " << type << " %r" << resultReg
                        << ", " << type << "* %r" << subExprReg << "\n";

            } else if(op == UO_PreInc || op == UO_PreDec) {

                // Generate code for pre-fix operators (e.g., ++x, --x)

                // TODO
                const Expr* subExpr = UO->getSubExpr();
                Visit(subExpr);

                unsigned int subExprReg = getReg(subExpr);
                std::string type = UO->getType().getAsString();

                unsigned int tmpReg = createReg(), consReg = createReg(), resultReg = createReg();

                char opStr = UnaryOperator::getOpcodeStr(op)[0];

                outFile_ << "%r" << tmpReg << " = load " << type
                         << ", " << type << "* %r" << subExprReg << "\n";
                outFile_ << "%r" << consReg << " = seti " << type << " 1\n";
                outFile_ << "%r" << resultReg << " = "
                         << type << " %r" << tmpReg
                         << " " << opStr << " "
                         << type << " %r" << consReg << "\n";
                outFile_ << "store " << type << " %r" << resultReg
                        << ", " << type << "* %r" << subExprReg << "\n";

                setReg(UO, subExprReg);

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

            const FunctionDecl* callee = call->getDirectCallee();
            std::string calleeType = callee->getReturnType().getAsString();
            std::string calleeName = callee->getNameAsString();

            if(calleeType == "void") {
                outFile_ << "call void @" << calleeName << "(";
            } else {
                unsigned int reg = createReg(call);
                outFile_ << "%r" << reg << " = call "
                         << calleeType << " @"
                         << calleeName << "(";
            }

            for(unsigned i = 0; i < call->getNumArgs(); i++) {
                if(i > 0) outFile_ << ", ";
                const Expr* arg = call->getArg(i);
                std::string argType = arg->getType().getAsString();
                unsigned int argReg = getReg(arg);
                outFile_ << argType << " %r" << argReg;
            }

            outFile_ << ")\n";
        }

        void VisitReturnStmt(const ReturnStmt* Stmt) {

            // Generate code for return statements

            // TODO
            if(const Expr* retValue = Stmt->getRetValue()) {
                Visit(retValue);
                outFile_ << "ret " << retValue->getType() << " %r" << getReg(retValue) << "\n";
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
            // Nothing to do here

        }

        void VisitIfStmt(const IfStmt* ifStmt) {

            // Generate code for if statement

            // TODO
            const unsigned int thenLabel = createLabel(), elseLabel = createLabel(), exitLabel = createLabel();
            const Expr* cond = ifStmt->getCond();
            const Stmt* elseStmt = ifStmt->getElse(), *then = ifStmt->getThen();

            Visit(cond);

            outFile_ << "br %r" << getReg(cond) << ", label L" << thenLabel << ", label L" << (elseStmt ? elseLabel : exitLabel) << "\n";
            outFile_ << "\nL" << thenLabel << ":\n";

            Visit(then);

            outFile_ << "br label L" << exitLabel << "\n";

            if(elseStmt) {
                outFile_ << "\nL" << elseLabel << ":\n";
                Visit(elseStmt);
                outFile_ << "br label L" << exitLabel << "\n";
            }

            outFile_ << "\nL" << exitLabel << ":\n";

        }

        void VisitConditionalOperator(const ConditionalOperator* CO) {

            // Generate code for ternary operator

            // TODO
            const auto trueLabel = createLabel(), falseLabel = createLabel(), exitLabel = createLabel();
            const Expr* cond = CO->getCond(), *trueExpr = CO->getTrueExpr(), *falseExpr = CO->getFalseExpr();

            Visit(cond);
            outFile_ << "br %r" << getReg(cond) << ", label L" << trueLabel << ", label L" << falseLabel << "\n";

            outFile_ << "\nL" << trueLabel << ":\n";
            Visit(trueExpr);
            outFile_ << "br label L" << exitLabel << "\n";

            outFile_ << "\nL" << falseLabel << ":\n";
            Visit(falseExpr);
            outFile_ << "br label L" << exitLabel << "\n";

            outFile_ << "\nL" << exitLabel << ":\n";

            const unsigned int reg = createReg(CO), trueReg = getReg(trueExpr), falseReg = getReg(falseExpr);
            outFile_ << "%r" << reg << " = phi(%r" << trueReg << ", %r" << falseReg << ")\n";

        }

        void VisitWhileStmt(const WhileStmt* whileStmt) {

            // Generate code for while loop

            // TODO
            const unsigned int condLabel = createLabel(), bodyLabel = createLabel(), exitLabel = createLabel();
            const Expr* cond = whileStmt->getCond();
            const Stmt* body = whileStmt->getBody();

            pushBreakLabel(exitLabel);
            pushContinueLabel(condLabel);

            outFile_ << "\nL" << condLabel << ":\n";
            Visit(cond);
            outFile_ << "br %r" << getReg(cond) << ", label L" << bodyLabel << ", label L" << exitLabel << "\n";

            outFile_ << "\nL" << bodyLabel << ":\n";
            Visit(body);
            outFile_ << "br label L" << condLabel << "\n";

            outFile_ << "\nL" << exitLabel << ":\n";

            popBreakLabel();
            popContinueLabel();

        }

        void VisitDoStmt(const DoStmt* doStmt) {

            // Generate code for do-while loop

            // TODO
            const unsigned int bodyLabel = createLabel(), condLabel = createLabel(), exitLabel = createLabel();
            const Stmt* body = doStmt->getBody();
            const Expr* cond = doStmt->getCond();

            pushBreakLabel(exitLabel);
            pushContinueLabel(condLabel);

            outFile_ << "\nL" << bodyLabel << ":\n";
            Visit(body);
            outFile_ << "br label L" << condLabel << "\n";

            outFile_ << "\nL" << condLabel << ":\n";
            Visit(cond);
            outFile_ << "br %r" << getReg(cond) << ", label L" << bodyLabel << ", label L" << exitLabel << "\n";

            outFile_ << "\nL" << exitLabel << ":\n";

            popBreakLabel();
            popContinueLabel();

        }

        void VisitForStmt(const ForStmt* forStmt) {

            // Generate code for for loop

            // TODO
            const unsigned int  condLabel = createLabel(), bodyLabel = createLabel(),
                                incLabel = createLabel(), exitLabel = createLabel();

            pushBreakLabel(exitLabel);
            pushContinueLabel(incLabel);

            if(const Stmt* init = forStmt->getInit()) {
                Visit(init);
            }

            outFile_ << "br label L" << condLabel << "\n";

            outFile_ << "\nL" << condLabel << ":\n";
            if(const Expr* cond = forStmt->getCond()) {
                Visit(cond);
                outFile_ << "br %r" << getReg(cond) << ", label L" << bodyLabel << ", label L" << exitLabel << "\n";
            } else {
                outFile_ << "br label L" << bodyLabel << "\n";
            }

            outFile_ << "\nL" << bodyLabel << ":\n";
            Visit(forStmt->getBody());
            outFile_ << "br label L" << incLabel << "\n";

            outFile_ << "\nL" << incLabel << ":\n";
            if(const Expr* inc = forStmt->getInc()) {
                Visit(inc);
            }
            outFile_ << "br label L" << condLabel << "\n";

            outFile_ << "\nL" << exitLabel << ":\n";

            popBreakLabel();
            popContinueLabel();

        }

        void VisitBreakStmt(const BreakStmt* B) {

            // Generate code for break statement

            // TODO
            outFile_ << "br label L" << getBreakLabel() << "\n";

        }

        void VisitContinueStmt(const ContinueStmt* C) {

            // Generate code for continue statement

            // TODO
            outFile_ << "br label L" << getContinueLabel() << "\n";

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
