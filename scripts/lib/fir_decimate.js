'use strict'

// Lowpass FIR filters data for 1/2, 1/3, ... 1/8 band.
// - 1 db non-linearity
// - 60 db supression
// - cutoff freq = newband * 8 / 7

const filter_tables = {
  2: require('./fir2_table.js'),
  3: require('./fir3_table.js'),
  4: require('./fir4_table.js'),
  8: require('./fir8_table.js')
}

module.exports = function fir_decimate (input, scale) {
  if (scale === 1) return input

  if (!filter_tables[scale]) {
    throw new Error(`No table for scale ${scale}`)
  }

  const ftable = filter_tables[scale]
  const taps = ftable.length
  const result = []

  for (let pos = 0; pos + taps < input.length;) {
    let res = 0
    for (let i = 0; i < taps; i++) res += input[pos + i] * ftable[i]
    result.push(res)
    pos += scale
  }

  return result.map(v => Math.round(v))
}
