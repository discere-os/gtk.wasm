/**
 * Type definitions for ${LIB_TITLE} WASM
 */

export interface ${LIB_UPPER}Module {
  _malloc: (size: number) => number
  _free: (ptr: number) => void
  HEAPU8: Uint8Array
  setValue: (ptr: number, value: number, type: string) => void
  getValue: (ptr: number, type: string) => number
}

export class ${LIB_UPPER}Error extends Error {
  constructor(message: string) {
    super(message)
    this.name = '${LIB_UPPER}Error'
  }
}
