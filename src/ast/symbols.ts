export enum DeclaredSymbolType {
    letOrConst = "letOrConst",
    function = "function",
    var = "var",
}

export type DeclaredSymbol = {
    type: DeclaredSymbolType;
    checkedIfUninitialized: boolean;
};

export class DeclaredSymbols {
    private symbols: Map<
        string,
        DeclaredSymbol
    >;

    constructor(...m: DeclaredSymbols[]) {
        this.symbols = new Map();
        m.forEach((ds) => ds.symbols.forEach((v, k) => this.symbols.set(k, v)));
    }

    has(name: string) {
        return this.symbols.has(name);
    }

    get(name: string) {
        return this.symbols.get(name);
    }

    set(name: string, value: DeclaredSymbol) {
        return this.symbols.set(name, value);
    }

    update(name: string, update: Partial<DeclaredSymbol>) {
        const oldValue = this.get(name);
        if (oldValue) {
            const newValue = { ...oldValue, ...update };
            return this.symbols.set(name, newValue);
        }
    }

    toSet() {
        return new Set(this.symbols.keys());
    }
}
