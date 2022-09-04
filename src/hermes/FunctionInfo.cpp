#include <hermes/FunctionInfo.h>

orangelabs::FunctionInfo::FunctionInfo(NAPIEnv env, NAPICallback callback,
                                       void *data)
    : env(env), callback(callback), data(data) {}

NAPIEnv orangelabs::FunctionInfo::getEnv() const {
  return env;
}

NAPICallback orangelabs::FunctionInfo::getCallback() const {
  return callback;
}

void *orangelabs::FunctionInfo::getData() const {
  return data;
}