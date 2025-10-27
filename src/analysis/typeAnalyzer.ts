import * as ts from "typescript";

import type { Node, Visitor } from "../ast/types";

export interface TypeInfo {
    type: string;
    properties?: Map<string, string>; // For objects
    elementType?: string; // For arrays
}

export class TypeAnalyzer {
    public symbolTable = new Map<string, TypeInfo>();
    private traverser: any; // Simplified for this example

    constructor(traverser: any) {
        this.traverser = traverser;
    }

    public analyze(ast: Node): Map<string, TypeInfo> {
        // In a real implementation, a traverser would be used to visit nodes
        // and populate the symbol table. This is a simplified representation.
        this.findVariableDeclarations(ast);
        return this.symbolTable;
    }

    private findVariableDeclarations(node: ts.Node) {
        if (ts.isVariableDeclaration(node)) {
            const name = node.name.getText();
            if (node.initializer) {
                const typeInfo = this.inferTypeFromInitializer(
                    node.initializer,
                );
                this.symbolTable.set(name, typeInfo);
            }
        }
        ts.forEachChild(node, this.findVariableDeclarations.bind(this));
    }

    private inferTypeFromInitializer(initializer: ts.Expression): TypeInfo {
        if (ts.isNumericLiteral(initializer)) {
            return {
                type: initializer.getText().includes(".") ? "double" : "int",
            };
        }
        if (ts.isStringLiteral(initializer)) {
            return { type: "std::string" };
        }
        if (ts.isArrayLiteralExpression(initializer)) {
            const elementTypes = new Set<string>();
            initializer.elements.forEach((element) => {
                const elementTypeInfo = this.inferTypeFromInitializer(element);
                elementTypes.add(elementTypeInfo.type);
            });

            if (elementTypes.size === 1) {
                return {
                    type: `std::vector<${[...elementTypes][0]}>`,
                    elementType: [...elementTypes][0],
                };
            } else {
                // Heterogeneous array
                const variantTypes = [...elementTypes].join(", ");
                return {
                    type: `std::vector<std::variant<${variantTypes}>>`,
                    elementType: `std::variant<${variantTypes}>`,
                };
            }
        }
        if (ts.isObjectLiteralExpression(initializer)) {
            const properties = new Map<string, string>();
            initializer.properties.forEach((prop) => {
                if (
                    ts.isPropertyAssignment(prop) && ts.isIdentifier(prop.name)
                ) {
                    const propName = prop.name.getText();
                    const propType =
                        this.inferTypeFromInitializer(prop.initializer).type;
                    properties.set(propName, propType);
                }
            });
            return { type: "struct", properties };
        }
        return { type: "auto" };
    }
}
