/**
 * Type definitions for Gtk WASM
 */

export interface GTKModule {
  _malloc: (size: number) => number
  _free: (ptr: number) => void
  HEAPU8: Uint8Array
  setValue: (ptr: number, value: number, type: string) => void
  getValue: (ptr: number, type: string) => number
}

export class GTKError extends Error {
  constructor(message: string) {
    super(message)
    this.name = 'GTKError'
  }
}
