const testCase = '(41.92 + 0.08);';
const expected = 42;
const actual = globalThis.addon.testNapiRun(testCase);

globalThis.assert.strictEqual(actual, expected);
// /string was expected/
globalThis.assert.throws(() => globalThis.addon.testNapiRun({ abc: 'def' }));

export { }