const valueDescriptor = Object.getOwnPropertyDescriptor(
  globalThis.addon.MyObject.prototype, 'value',
);
const valueReadonlyDescriptor = Object.getOwnPropertyDescriptor(
  globalThis.addon.MyObject.prototype, 'valueReadonly',
);
const plusOneDescriptor = Object.getOwnPropertyDescriptor(
  globalThis.addon.MyObject.prototype, 'plusOne',
);
if (valueDescriptor) {
  globalThis.assert.strictEqual(typeof valueDescriptor.get, 'function');
  globalThis.assert.strictEqual(typeof valueDescriptor.set, 'function');
  globalThis.assert.strictEqual(valueDescriptor.value, undefined);
  globalThis.assert.strictEqual(valueDescriptor.enumerable, false);
  globalThis.assert.strictEqual(valueDescriptor.configurable, false);
} else {
  globalThis.assert(false);
}

if (valueReadonlyDescriptor) {
  globalThis.assert.strictEqual(typeof valueReadonlyDescriptor.get, 'function');
  globalThis.assert.strictEqual(valueReadonlyDescriptor.set, undefined);
  globalThis.assert.strictEqual(valueReadonlyDescriptor.value, undefined);
  globalThis.assert.strictEqual(valueReadonlyDescriptor.enumerable, false);
  globalThis.assert.strictEqual(valueReadonlyDescriptor.configurable, false);
} else {
  globalThis.assert(false);
}

if (plusOneDescriptor) {
  globalThis.assert.strictEqual(plusOneDescriptor.get, undefined);
  globalThis.assert.strictEqual(plusOneDescriptor.set, undefined);
  globalThis.assert.strictEqual(typeof plusOneDescriptor.value, 'function');
  globalThis.assert.strictEqual(plusOneDescriptor.enumerable, false);
  globalThis.assert.strictEqual(plusOneDescriptor.configurable, false);
} else {
  globalThis.assert(false);
}

const obj = new globalThis.addon.MyObject(9);
globalThis.assert.strictEqual(obj.value, 9);
obj.value = 10;
globalThis.assert.strictEqual(obj.value, 10);
globalThis.assert.strictEqual(obj.valueReadonly, 10);
// Object.defineProperty 定义的只读属性不会抛出异常，只有 V8 定义的才会
// globalThis.assert.throws(() => { obj.valueReadonly = 14; }, getterOnlyErrorRE);
globalThis.assert.strictEqual(obj.plusOne(), 11);
globalThis.assert.strictEqual(obj.plusOne(), 12);
globalThis.assert.strictEqual(obj.plusOne(), 13);

globalThis.assert.strictEqual(obj.multiply().value, 13);
globalThis.assert.strictEqual(obj.multiply(10).value, 130);

const newobj = obj.multiply(-1);
globalThis.assert.strictEqual(newobj.value, -13);
globalThis.assert.strictEqual(newobj.valueReadonly, -13);
globalThis.assert.notStrictEqual(obj, newobj);

export { }