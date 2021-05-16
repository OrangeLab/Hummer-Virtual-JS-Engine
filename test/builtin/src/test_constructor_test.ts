const test_object = new globalThis.addon();

globalThis.assert.strictEqual(test_object.echo('hello'), 'hello');

test_object.readwriteValue = 1;
globalThis.assert.strictEqual(test_object.readwriteValue, 1);
test_object.readwriteValue = 2;
globalThis.assert.strictEqual(test_object.readwriteValue, 2);

// globalThis.assert.throws(() => { test_object.readonlyValue = 3; });

globalThis.assert.ok(test_object.hiddenValue);

// Properties with napi_enumerable attribute should be enumerable.
const propertyNames: string[] = [];
for (const name in test_object) {
  propertyNames.push(name);
}
globalThis.assert.ok(propertyNames.includes('echo'));
globalThis.assert.ok(propertyNames.includes('readwriteValue'));
globalThis.assert.ok(propertyNames.includes('readonlyValue'));
globalThis.assert.ok(!propertyNames.includes('hiddenValue'));
globalThis.assert.ok(!propertyNames.includes('readwriteAccessor1'));
globalThis.assert.ok(!propertyNames.includes('readwriteAccessor2'));
globalThis.assert.ok(!propertyNames.includes('readonlyAccessor1'));
globalThis.assert.ok(!propertyNames.includes('readonlyAccessor2'));

// The napi_writable attribute should be ignored for accessors.
test_object.readwriteAccessor1 = 1;
globalThis.assert.strictEqual(test_object.readwriteAccessor1, 1);
globalThis.assert.strictEqual(test_object.readonlyAccessor1, 1);
// globalThis.assert.throws(() => { test_object.readonlyAccessor1 = 3; });
test_object.readwriteAccessor2 = 2;
globalThis.assert.strictEqual(test_object.readwriteAccessor2, 2);
globalThis.assert.strictEqual(test_object.readonlyAccessor2, 2);
// globalThis.assert.throws(() => { test_object.readonlyAccessor2 = 3; });

// Validate that static properties are on the class as opposed
// to the instance
globalThis.assert.strictEqual(globalThis.addon.staticReadonlyAccessor1, 10);
globalThis.assert.strictEqual(test_object.staticReadonlyAccessor1, undefined);

// Verify that passing NULL to napi_define_class() results in the correct
// error.
// globalThis.assert.deepStrictEqual(globalThis.addon.TestDefineClass(), {
//   envIsNull: 'Invalid argument',
//   nameIsNull: 'Invalid argument',
//   cbIsNull: 'Invalid argument',
//   cbDataIsNull: 'napi_ok',
//   propertiesIsNull: 'Invalid argument',
//   resultIsNull: 'Invalid argument'
// });

export { }