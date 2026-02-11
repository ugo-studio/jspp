import type ts from "typescript";

export enum DeclarationType {
    var = "var",
    let = "let",
    const = "const",
    function = "function",
    class = "class",
    enum = "enum",
}

export type SymbolChecks = {
    initialized?: boolean;
};

export type SymbolFeatures = {
    native?: {
        type: "lambda";
        name: string;
        parameters: ts.ParameterDeclaration[];
    };
    isAsync?: boolean;
    isGenerator?: boolean;
};

export class DeclaredSymbol {
    type: DeclarationType;
    checks: SymbolChecks;
    features: SymbolFeatures;

    constructor(type: DeclarationType) {
        this.type = type;
        this.checks = { initialized: false };
        this.features = {};
    }

    get isMutable() {
        return this.type === DeclarationType.let ||
            this.type === DeclarationType.var;
    }

    updateChecked(update: Partial<SymbolChecks>) {
        this.checks = {
            ...this.checks,
            ...update,
        };
    }

    updateFeatures(
        update: Partial<SymbolFeatures>,
    ) {
        this.features = {
            ...this.features,
            ...update,
        };
    }
}

export class DeclaredSymbols {
    private symbols: Map<
        string,
        DeclaredSymbol
    >;

    constructor(...m: DeclaredSymbols[]) {
        this.symbols = new Map();
        m.forEach((ds) => ds.symbols.forEach((v, k) => this.symbols.set(k, v)));
    }

    get names() {
        return new Set(this.symbols.keys());
    }

    has(name: string) {
        return this.symbols.has(name);
    }

    get(name: string) {
        return this.symbols.get(name);
    }

    add(name: string, value: {
        type: DeclarationType;
        checks?: Partial<SymbolChecks>;
        features?: Partial<SymbolFeatures>;
    }) {
        const sym = new DeclaredSymbol(value.type);
        if (value.checks) sym.updateChecked(value.checks);
        if (value.features) sym.updateFeatures(value.features);
        return this.symbols.set(name, sym);
    }

    update(
        name: string,
        update: Partial<
            {
                type: DeclarationType;
                checks: Partial<SymbolChecks>;
                features: Partial<SymbolFeatures>;
            }
        >,
    ) {
        const sym = this.get(name);
        if (sym) {
            if (update.type) sym.type = update.type;
            if (update.checks) sym.updateChecked(update.checks);
            if (update.features) {
                sym.updateFeatures(update.features);
            }
            return this.symbols.set(name, sym);
        }
    }

    set(
        name: string,
        update: Partial<
            {
                type: DeclarationType;
                checks: SymbolChecks;
                features: SymbolFeatures;
            }
        >,
    ) {
        const sym = this.get(name);
        if (sym) {
            if (update.type) sym.type = update.type;
            if (update.checks) sym.checks = update.checks;
            if (update.features) sym.features = update.features;
            return this.symbols.set(name, sym);
        }
    }
}
