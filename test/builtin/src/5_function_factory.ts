const fn = globalThis.addon();
globalThis.assert.strictEqual(fn(), 'hello world'); // 'hello world'

export { }