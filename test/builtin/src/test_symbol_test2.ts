const fooSym = globalThis.addon.New('foo');
const myObj: { foo?: string } = {};
myObj.foo = 'bar';
myObj[fooSym] = 'baz';
Object.keys(myObj); // -> [ 'foo' ]
Object.getOwnPropertyNames(myObj); // -> [ 'foo' ]
Object.getOwnPropertySymbols(myObj); // -> [ Symbol(foo) ]
globalThis.assert.strictEqual(Object.getOwnPropertySymbols(myObj)[0], fooSym);

export { }