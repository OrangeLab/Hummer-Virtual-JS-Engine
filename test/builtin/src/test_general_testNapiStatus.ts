globalThis.addon.createNapiError();
globalThis.assert(globalThis.addon.testNapiErrorCleanup(), 'napi_status cleaned up for second call');

export { }