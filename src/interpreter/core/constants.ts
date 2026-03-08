import ts from "typescript";

const booleanOperators = [
    ts.SyntaxKind.EqualsEqualsEqualsToken,
    ts.SyntaxKind.EqualsEqualsToken,
    ts.SyntaxKind.ExclamationEqualsEqualsToken,
    ts.SyntaxKind.ExclamationEqualsToken,
    ts.SyntaxKind.InstanceOfKeyword,
    ts.SyntaxKind.InKeyword,
    ts.SyntaxKind.LessThanToken,
    ts.SyntaxKind.LessThanEqualsToken,
    ts.SyntaxKind.GreaterThanToken,
    ts.SyntaxKind.GreaterThanEqualsToken,
];

export const constants = Object.freeze({
    booleanOperators,
});
