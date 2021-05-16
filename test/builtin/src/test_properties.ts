globalThis.assert.strictEqual(globalThis.addon.echo('hello'), 'hello');

globalThis.addon.readwriteValue = 1;
globalThis.assert.strictEqual(globalThis.addon.readwriteValue, 1);
globalThis.addon.readwriteValue = 2;
globalThis.assert.strictEqual(globalThis.addon.readwriteValue, 2);

// globalThis.assert.throws(() => { globalThis.addon.readonlyValue = 3; }, readonlyErrorRE);

globalThis.assert.ok(globalThis.addon.hiddenValue);

// Properties with napi_enumerable attribute should be enumerable.
const propertyNames: string[] = [];
for (const name in globalThis.addon) {
  propertyNames.push(name);
}
globalThis.assert.ok(propertyNames.includes('echo'));
globalThis.assert.ok(propertyNames.includes('readwriteValue'));
globalThis.assert.ok(propertyNames.includes('readonlyValue'));
globalThis.assert.ok(!propertyNames.includes('hiddenValue'));
globalThis.assert.ok(propertyNames.includes('NameKeyValue'));
globalThis.assert.ok(!propertyNames.includes('readwriteAccessor1'));
globalThis.assert.ok(!propertyNames.includes('readwriteAccessor2'));
globalThis.assert.ok(!propertyNames.includes('readonlyAccessor1'));
globalThis.assert.ok(!propertyNames.includes('readonlyAccessor2'));

// Validate property created with symbol
const start = 'Symbol('.length;
const end = start + 'NameKeySymbol'.length;
const symbolDescription = String(Object.getOwnPropertySymbols(globalThis.addon)[0]).slice(start, end);
globalThis.assert.strictEqual(symbolDescription, 'NameKeySymbol');

// The napi_writable attribute should be ignored for accessors.
const readwriteAccessor1Descriptor = Object.getOwnPropertyDescriptor(globalThis.addon, 'readwriteAccessor1');
const readonlyAccessor1Descriptor = Object.getOwnPropertyDescriptor(globalThis.addon, 'readonlyAccessor1');
if (readwriteAccessor1Descriptor) {
  globalThis.assert.ok(readwriteAccessor1Descriptor.get != null);
  globalThis.assert.ok(readwriteAccessor1Descriptor.set != null);
  globalThis.assert.ok(readwriteAccessor1Descriptor.value === undefined);
} else {
  globalThis.assert(false);
}

if (readonlyAccessor1Descriptor) {
  globalThis.assert.ok(readonlyAccessor1Descriptor.get != null);
  globalThis.assert.ok(readonlyAccessor1Descriptor.set === undefined);
  globalThis.assert.ok(readonlyAccessor1Descriptor.value === undefined);
} else {
  globalThis.assert(false);
}

globalThis.addon.readwriteAccessor1 = 1;
globalThis.assert.strictEqual(globalThis.addon.readwriteAccessor1, 1);
globalThis.assert.strictEqual(globalThis.addon.readonlyAccessor1, 1);
// globalThis.assert.throws(() => { globalThis.addon.readonlyAccessor1 = 3; }, getterOnlyErrorRE);
globalThis.addon.readwriteAccessor2 = 2;
globalThis.assert.strictEqual(globalThis.addon.readwriteAccessor2, 2);
globalThis.assert.strictEqual(globalThis.addon.readonlyAccessor2, 2);
// globalThis.assert.throws(() => { globalThis.addon.readonlyAccessor2 = 3; }, getterOnlyErrorRE);

globalThis.assert.strictEqual(globalThis.addon.hasNamedProperty(globalThis.addon, 'echo'), true);
globalThis.assert.strictEqual(globalThis.addon.hasNamedProperty(globalThis.addon, 'hiddenValue'),
  true);
globalThis.assert.strictEqual(globalThis.addon.hasNamedProperty(globalThis.addon, 'doesnotexist'),
  false);

export { }