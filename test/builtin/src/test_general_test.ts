const val1 = '1';
const val2 = 1;
const val3 = 1;

class BaseClass {
}

class ExtendedClass extends BaseClass {
}

const baseObject = new BaseClass();
const extendedObject = new ExtendedClass();

// Test napi_strict_equals
globalThis.assert.ok(globalThis.addon.testStrictEquals(val1, val1));
globalThis.assert.strictEqual(globalThis.addon.testStrictEquals(val1, val2), false);
globalThis.assert.ok(globalThis.addon.testStrictEquals(val2, val3));

// Test napi_get_prototype
globalThis.assert.strictEqual(globalThis.addon.testGetPrototype(baseObject),
  Object.getPrototypeOf(baseObject));
globalThis.assert.strictEqual(globalThis.addon.testGetPrototype(extendedObject),
  Object.getPrototypeOf(extendedObject));
// Prototypes for base and extended should be different.
globalThis.assert.notStrictEqual(globalThis.addon.testGetPrototype(baseObject),
  globalThis.addon.testGetPrototype(extendedObject));

// Test version management functions
// globalThis.assert.strictEqual(globalThis.addon.testGetVersion(), 7);

[
  123,
  'test string',
  // eslint-disable-next-line @typescript-eslint/no-empty-function
  function () { },
  new Object(),
  true,
  undefined,
  Symbol(),
].forEach((val) => {
  globalThis.assert.strictEqual(globalThis.addon.testNapiTypeof(val), typeof val);
});

// Since typeof in js return object need to validate specific case
// for null
globalThis.assert.strictEqual(globalThis.addon.testNapiTypeof(null), 'null');

// globalThis.assert that wrapping twice fails.
const x = {};
globalThis.addon.wrap(x);
globalThis.assert.throws(() => globalThis.addon.wrap(x),
  //   { name: 'Error', message: 'Invalid argument' }
);
// Clean up here, otherwise derefItemWasCalled() will be polluted.
globalThis.addon.removeWrap(x);

// Ensure that wrapping, removing the wrap, and then wrapping again works.
const y = {};
globalThis.addon.wrap(y);
globalThis.addon.removeWrap(y);
// Wrapping twice succeeds if a remove_wrap() separates the instances
globalThis.addon.wrap(y);
// Clean up here, otherwise derefItemWasCalled() will be polluted.
globalThis.addon.removeWrap(y);

// Test napi_adjust_external_memory
// const adjustedValue = globalThis.addon.testAdjustExternalMemory();
// globalThis.assert.strictEqual(typeof adjustedValue, 'number');
// globalThis.assert(adjustedValue > 0);

// async function runGCTests() {
//   // Ensure that garbage collecting an object with a wrapped native item results
//   // in the finalize callback being called.
//   globalThis.assert.strictEqual(globalThis.addon.derefItemWasCalled(), false);

//   (() => globalThis.addon.wrap({}))();
//   await common.gcUntil('deref_item() was called upon garbage collecting a ' +
//                        'wrapped object.',
//                        () => globalThis.addon.derefItemWasCalled());

//   // Ensure that removing a wrap and garbage collecting does not fire the
//   // finalize callback.
//   let z = {};
//   globalThis.addon.testFinalizeWrap(z);
//   globalThis.addon.removeWrap(z);
//   z = null;
//   await common.gcUntil(
//     'finalize callback was not called upon garbage collection.',
//     () => (!globalThis.addon.finalizeWasCalled()));
// }
// runGCTests();

export { }