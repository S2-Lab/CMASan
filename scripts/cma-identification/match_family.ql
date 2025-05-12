/**
 * This is an automatically generated file
 * @name CMA Identification Engine
 * @kind problem
 * @problem.severity warning
 * @id cpp/example/familymatching
 */

import cpp

external string getAnExternal();

predicate isAllocFunction(Function f) {
    exists(string str | str = getAnExternal() |
        f.getName().toString() = str.splitAt(s(), 0)
        and f.getFile().toString() = str.splitAt(s(), 1)
        and f.getLocation().getStartLine().toString() = str.splitAt(s(), 2)
    )
}

class AllocFunction extends Function {
    AllocFunction () {
        isAllocFunction(this)
    }
}

string s() {
    result = "|"
}

string formatFunction(Function f) {
    result = f.getName() + s() + f.getFile().getAbsolutePath().toString() + s() + f.getDefinitionLocation().getStartLine().toString() + s() + f.getDefinition().getNumberOfLines().toString()
}

from AllocFunction alloc_f, Function family_f
where
    alloc_f != family_f
    and (
        alloc_f.getDeclaringType() = family_f.getDeclaringType() // C++ class/struct member function
        or
        (
            (not alloc_f.isMember() and not family_f.isMember() and alloc_f.getNamespace() = family_f.getNamespace())
            and
            (
                alloc_f.getFile() = family_f.getFile()
                or alloc_f.getDefinition().getFile() = family_f.getADeclaration().getFile()
                or alloc_f.getADeclaration().getFile() = family_f.getDefinition().getFile()
                or alloc_f.getADeclaration().getFile() = family_f.getADeclaration().getFile()
            )
        )
    )
select alloc_f, formatFunction(alloc_f) + s() + formatFunction(family_f)