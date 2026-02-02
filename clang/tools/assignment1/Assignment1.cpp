
#include "clang/AST/AST.h"
#include "clang/AST/ASTConsumer.h"
#include "clang/AST/RecursiveASTVisitor.h"
#include "clang/Frontend/ASTConsumers.h"
#include "clang/Frontend/FrontendActions.h"
#include "clang/Frontend/CompilerInstance.h"
#include "clang/Tooling/CommonOptionsParser.h"
#include "clang/Tooling/Tooling.h"
#include "clang/Rewrite/Core/Rewriter.h"

#include <string>
#include <sstream>

using namespace clang::driver;
using namespace clang::tooling;
using namespace clang;

class Assignment1Transform : public RecursiveASTVisitor<Assignment1Transform> {

    public:

        Assignment1Transform(Rewriter& rewriter) : rewriter_(rewriter) { }

        bool VisitFunctionDecl(FunctionDecl *funcDecl) {

            // If the function is main and it has a body, insert code at the beginning of the function that declares and initializes the controller

            // TODO

            if (funcDecl->getName() == "main") {
              auto bodyStartLoc = funcDecl->getBody()->getBeginLoc();
              rewriter_.InsertTextAfterToken(bodyStartLoc, """\n    struct __Controller __c;\n"
                                                       "    __c.skipThen = 0;\n"
                                                       "    __c.skipElse = 0;\n"
                                                       "    __c.skipWhileLoops = 0;\n"
                                                       "    __c.skipDoWhileLoops = 0;\n"
                                                       "    __c.skipForLoops = 0;\n"
                                                       "    __c.skipBreaks = 0;\n"
                                                       "    __c.skipContinues = 0;\n"
                                                       "    __c.skipFunctionName = \"\";""");
            }

            // If the function is not main, insert a parameter and the end that takes the controller
            // Assume that the function already has at least one parameter

            // TODO

            else {
              auto paramSrcRange = funcDecl->getParametersSourceRange().getEnd();
              rewriter_.InsertTextAfterToken(paramSrcRange, ", struct __Controller __c");
            }

            return true;

        }

        bool VisitIfStmt(IfStmt* ifStmt) {

            // Modify the condition to also check whether or not to skip the then statement

            // TODO

            auto condEndLoc = ifStmt->getCond()->getEndLoc();
            auto condStartLoc = ifStmt->getCond()->getBeginLoc();
            rewriter_.InsertTextBefore(condStartLoc, "(");
            rewriter_.InsertTextAfterToken(condEndLoc, ") && !__c.skipThen");

            // If an else statement exist, guard it with an if statement that checks whether or not to skip else

            // TODO

            if (ifStmt->getElse() != NULL) {
              auto elseLoc = ifStmt->getElseLoc();
              rewriter_.InsertTextAfterToken(elseLoc, " if(!__c.skipElse)");
            }

            return true;

        }

        bool VisitWhileStmt(WhileStmt* whileStmt) {

            // Modify the condition to also check whether or not to skip while loops

            // TODO
            auto condEndLoc = whileStmt->getCond()->getEndLoc();
            auto condStartLoc = whileStmt->getCond()->getBeginLoc();
            rewriter_.InsertTextBefore(condStartLoc, "(");
            rewriter_.InsertTextAfterToken(condEndLoc, ") && !__c.skipWhileLoops");

            return true;

        }

        bool VisitDoStmt(DoStmt* doStmt) {

            // Guard the do-while loop with an if statement that checks whether or not skip do-while loops

            // TODO

            auto doStartLoc = doStmt->getBeginLoc();
            rewriter_.InsertTextBefore(doStartLoc, "if(!__c.skipDoWhileLoops)\n     ");

            return true;

        }

        bool VisitForStmt(ForStmt* forStmt) {

            // Modify the condition to also check whether or not to skip for loops

            // TODO

            auto forStartLoc = forStmt->getCond()->getBeginLoc();
            auto forEndLoc = forStmt->getCond()->getEndLoc();
            rewriter_.InsertTextBefore(forStartLoc, "(");
            rewriter_.InsertTextAfterToken(forEndLoc, ") && !__c.skipForLoops");

            return true;

        }

        bool VisitBreakStmt(BreakStmt* breakStmt) {

            // Guard the break statement with an if statement the checks whether or not to skip break statements

            // TODO

            auto breakBeginLoc = breakStmt->getBeginLoc();
            rewriter_.InsertTextBefore(breakBeginLoc, "if(!__c.skipBreaks) ");

            return true;
        }

        bool VisitContinueStmt(ContinueStmt* continueStmt) {

            // Guard the continue statement with an if statement the checks whether or not to skip continue statements

            // TODO

            auto continueBeginLoc = continueStmt->getBeginLoc();
            rewriter_.InsertTextBefore(continueBeginLoc, "if(!__c.skipContinues) ");

            return true;
        }

        bool VisitCallExpr(CallExpr* callExpr) {

            // Add the controller as an argument at the end
            // Assume a functions is called directly
            // Assume that the function called already has at least one argument

            // TODO

            auto lastArgEndLoc = callExpr->getArgs()[callExpr->getNumArgs() - 1]->getEndLoc();
            rewriter_.InsertTextAfterToken(lastArgEndLoc, ", __c");


            // Guard direct function calls with a check that skips calls to functions with a particular name
            // Assume a functions is called directly
            // Assume the call expression is its own statement, not a subexpression of another statement

            // TODO

            auto callBeginLoc = callExpr->getBeginLoc();
            rewriter_.InsertTextBefore(callBeginLoc, "if(strcmp(__c.skipFunctionName, \"" + callExpr->getDirectCallee()->getNameAsString() + "\") != 0) ");

            return true;

        }

    private:

        Rewriter &rewriter_;

};

static llvm::cl::OptionCategory Assignment1Category("Assignment 1 Command Line Options");
static llvm::cl::opt<std::string>
    outFileName("o",
        llvm::cl::desc("Output file name"),
        llvm::cl::init("transformed.c"),
        llvm::cl::cat(Assignment1Category));

class Assignment1ASTConsumer : public ASTConsumer {

    public:

        Assignment1ASTConsumer(Rewriter &rewriter)
            : rewriter_(rewriter) {}

        virtual void HandleTranslationUnit(ASTContext &Context) {
            TranslationUnitDecl* TU = Context.getTranslationUnitDecl();
            Assignment1Transform transform(rewriter_);
            transform.TraverseDecl(TU);
        }

    private:

        Rewriter& rewriter_;

};

class Assignment1Action : public ASTFrontendAction {

    public:

        Assignment1Action() {}

        std::unique_ptr<ASTConsumer> CreateASTConsumer(CompilerInstance &CI, StringRef file) override {
            rewriter_.setSourceMgr(CI.getSourceManager(), CI.getLangOpts());
            return std::make_unique<Assignment1ASTConsumer>(rewriter_);
        }

        void EndSourceFileAction() override {
            std::error_code EC;

            llvm::sys::fs::OpenFlags flags = llvm::sys::fs::OF_Text;
            llvm::raw_fd_ostream FileStream(outFileName, EC, flags);
            if (EC) {
                llvm::errs() << "Error: Could not write to " << EC.message() << "\n";
            } else {

                // Add a declaration of the controller structure to the begining of the file

                // TODO

//                FileStream << ""; // Add text to the beginning of the file here

                FileStream  << "#include <string.h>\n\n"
                            << "struct __Controller {\n"
                            << "  unsigned int skipThen;\n"
                            << "  unsigned int skipElse;\n"
                            << "  unsigned int skipWhileLoops;\n"
                            << "  unsigned int skipDoWhileLoops;\n"
                            << "  unsigned int skipForLoops;\n"
                            << "  unsigned int skipBreaks;\n"
                            << "  unsigned int skipContinues;\n"
                            << "  char* skipFunctionName;\n"
                            << "};\n";




                SourceManager &SM = rewriter_.getSourceMgr();
                rewriter_.getEditBuffer(SM.getMainFileID()).write(FileStream);

            }
        }

    private:
        Rewriter rewriter_;

};

int main(int argc, const char **argv) {
    auto ExpectedParser = tooling::CommonOptionsParser::create(argc, argv, Assignment1Category);
    if (!ExpectedParser) {
        llvm::errs() << ExpectedParser.takeError();
        return 1;
    }
    tooling::CommonOptionsParser &OptionsParser = ExpectedParser.get();
    ClangTool Tool(OptionsParser.getCompilations(), OptionsParser.getSourcePathList());
    return Tool.run(newFrontendActionFactory<Assignment1Action>().get());
}

