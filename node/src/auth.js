export function authCheck(authList, name, password, cb) {
  // console.log('authList, name, password', authList, name, password)
  cb(authList[name] === password)
}
