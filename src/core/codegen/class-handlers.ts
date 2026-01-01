import ts from "typescript";

import { CodeGenerator } from "./";
import { visitObjectPropertyName } from "./expression-handlers";
import type { VisitContext } from "./visitor";

export function visitClassDeclaration(
    this: CodeGenerator,
    node: ts.ClassDeclaration,
    context: VisitContext,
): string {
    const className = node.name!.getText();

    // Check extends
    let extendsExpr: ts.Expression | null = null;
    if (node.heritageClauses) {
        for (const clause of node.heritageClauses) {
            if (
                clause.token === ts.SyntaxKind.ExtendsKeyword &&
                clause.types.length > 0
            ) {
                extendsExpr = clause.types[0]!.expression;
            }
        }
    }

    let parentName = "";
    if (extendsExpr) {
        parentName = this.visit(extendsExpr, context);
        if (ts.isIdentifier(extendsExpr)) {
            const scope = this.getScopeForNode(extendsExpr);
            const typeInfo = this.typeAnalyzer.scopeManager.lookupFromScope(
                extendsExpr.getText(),
                scope,
            );
            if (typeInfo) {
                parentName = this.getDerefCode(
                    parentName,
                    this.getJsVarName(extendsExpr),
                    context,
                    typeInfo,
                );
            }
        }
    }

    const classContext: VisitContext = {
        ...context,
        superClassVar: parentName,
    };

    // 1. Constructor
    const constructor = node.members.find(
        ts.isConstructorDeclaration,
    ) as ts.ConstructorDeclaration;
    let constructorLambda = "";

    if (constructor) {
        constructorLambda = this.generateLambda(constructor, {
            ...classContext,
            isInsideFunction: true,
            lambdaName: className,
        }, { isClass: true });
    } else {
        // Default constructor
        if (parentName) {
            constructorLambda =
                `jspp::AnyValue::make_class([=](const jspp::AnyValue& ${this.globalThisVar}, std::span<const jspp::AnyValue> args) mutable -> jspp::AnyValue {
                 auto __parent = ${parentName};
                 __parent.call(${this.globalThisVar}, args, "super");
                 return jspp::Constants::UNDEFINED;
             }, "${className}")`;
        } else {
            constructorLambda =
                `jspp::AnyValue::make_class([=](const jspp::AnyValue& ${this.globalThisVar}, std::span<const jspp::AnyValue> args) mutable -> jspp::AnyValue {
                 return jspp::Constants::UNDEFINED;
             }, "${className}")`;
        }
    }

    let code = `${this.indent()}*${className} = ${constructorLambda};\n`;

    // Set prototype of class (static inheritance) and prototype object (instance inheritance)
    if (parentName) {
        code +=
            `${this.indent()}(*${className}).set_prototype(${parentName});\n`;
        code +=
            `${this.indent()}(*${className}).get_own_property("prototype").set_prototype(${parentName}.get_own_property("prototype"));\n`;
    } else {
        code +=
            `${this.indent()}(*${className}).get_own_property("prototype").set_prototype(::Object.get_own_property("prototype"));\n`;
    }

    // Members
    for (const member of node.members) {
        if (ts.isMethodDeclaration(member)) {
            const methodName = visitObjectPropertyName.call(this, member.name, {
                ...context,
                isObjectLiteralExpression: true, // Reuse this flag to handle computed properties
            });
            const isStatic = member.modifiers?.some((m) =>
                m.kind === ts.SyntaxKind.StaticKeyword
            );

            const methodLambda = this.generateLambda(member, {
                ...classContext,
                isInsideFunction: true,
            });

            if (isStatic) {
                code +=
                    `${this.indent()}(*${className}).set_own_property(${methodName}, ${methodLambda});\n`;
            } else {
                code +=
                    `${this.indent()}(*${className}).get_own_property("prototype").set_own_property(${methodName}, ${methodLambda});\n`;
            }
        } else if (ts.isGetAccessor(member)) {
            const methodName = visitObjectPropertyName.call(this, member.name, {
                ...context,
                isObjectLiteralExpression: true,
            });
            const isStatic = member.modifiers?.some((m) =>
                m.kind === ts.SyntaxKind.StaticKeyword
            );
            const lambda = this.generateLambda(member, {
                ...classContext,
                isInsideFunction: true,
            });

            if (isStatic) {
                code +=
                    `${this.indent()}(*${className}).define_getter(${methodName}, ${lambda});\n`;
            } else {
                code +=
                    `${this.indent()}(*${className}).get_own_property("prototype").define_getter(${methodName}, ${lambda});\n`;
            }
        } else if (ts.isSetAccessor(member)) {
            const methodName = visitObjectPropertyName.call(this, member.name, {
                ...context,
                isObjectLiteralExpression: true,
            });
            const isStatic = member.modifiers?.some((m) =>
                m.kind === ts.SyntaxKind.StaticKeyword
            );
            const lambda = this.generateLambda(member, {
                ...classContext,
                isInsideFunction: true,
            });

            if (isStatic) {
                code +=
                    `${this.indent()}(*${className}).define_setter(${methodName}, ${lambda});\n`;
            } else {
                code +=
                    `${this.indent()}(*${className}).get_own_property("prototype").define_setter(${methodName}, ${lambda});\n`;
            }
        }
    }

    return code;
}
