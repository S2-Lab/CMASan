/**
 * This is an automatically generated file
 * @name CMA Identification Engine
 * @kind problem
 * @problem.severity warning
 * @id cpp/example/cmaidentification
 */

import cpp

/**
 * Rules
 */
predicate satisfyRulePrototype(Function f) {
    f.getName().regexpMatch(".*(alloc|Alloc|((get|Get|acquire|Acquire|make|Make).*(Memory|memory|space|Space))).*")
    and f.getUnspecifiedType() instanceof PointerType
    and f.getAParameter().getUnspecifiedType() instanceof IntegralType
}

predicate isStorageDeclaration(Declaration decl) {
    decl.getName().regexpMatch(".*(heap|Heap|free|Free|buddy|Buddy|arena|Arena|slab|Slab|storage|Storage|memory|Memory|block|Block|area|Area|pool|Pool|chunk|Chunk|cache|Cache|base|Base|head|Head).*")
}
class StorageVariable extends Variable {
    StorageVariable() { isStorageDeclaration(this) }
}

class StorageFunction extends Function {
    StorageFunction() { isStorageDeclaration(this) }
}

predicate satisfyRuleStorage(Function f) {
    exists(ReturnStmt rs, Variable returnVar |
        rs.getControlFlowScope() = f 
        and
        returnVar=rs.getExpr().(VariableAccess).getTarget()
        and
        exists(ControlFlowNode cfn |
            cfn=rs.getExpr().getAPredecessor+()
            and (
                exists(VariableAccess va | cfn = va and va.getTarget() != returnVar and va.getTarget() instanceof StorageVariable)
                or
                exists(FunctionCall fc | cfn = fc and (fc.getTarget() instanceof StorageFunction or fc.getASuccessor().(FieldAccess).getParent+().(Initializer).getDeclaration() = returnVar))
            )
        )
    )
}

predicate satisfyRuleSize(Function f) {
    exists(Parameter p, int i, Access a | 
        p = f.getParameter(i) and p.getUnspecifiedType() instanceof IntegralType 
        and a = p.getAnAccess()
        and exists(Expr parent | parent = a.getParent+() |
            // size parameter added with alignment
            parent instanceof AddExpr
            or parent instanceof SubExpr
            or parent instanceof AssignAddExpr
            or parent instanceof AssignSubExpr
            // size parameter compared to others
            or parent instanceof ComparisonOperation
            // size parameter added to internal storage to get pointer
            or parent instanceof PointerAddExpr
            or parent instanceof PointerSubExpr
            or parent instanceof AssignPointerAddExpr
            or parent instanceof AssignPointerSubExpr
        )
    )
}

predicate antiFunc(Function f) {
    f.getFile().getAbsolutePath().regexpMatch(".*llvm.*|.*LLVM.*|.*/usr/include/c++.*") // Instrumenting LLVM is discouraged by ASan
    or
    f.getFile().getAbsolutePath().regexpMatch(".*jemalloc.*|.*tcmalloc.*") and not f.getFile().getAbsolutePath().regexpMatch(".*jemalloc.c*|.*tcmalloc.cc*")
}

class CMAFunction extends Function {
    CMAFunction () {
        satisfyRulePrototype(this)
        and (
            satisfyRuleSize(this)
            or satisfyRuleStorage(this)
        )
        and not antiFunc(this)
    }
}

string s() {
    result = "|"
}

string formatFunction(Function f) {
    result = f.getName() + s() + f.getFile().getAbsolutePath().toString() + s() + f.getDefinitionLocation().getStartLine().toString() + s() + f.getDefinition().getNumberOfLines().toString()
}

from CMAFunction f
select f, formatFunction(f)