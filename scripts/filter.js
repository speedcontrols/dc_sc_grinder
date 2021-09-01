#!/usr/bin/env node

'use strict'

const SAMPLE_FREQUENCY     = 250000
const MIN_FILTER_FREQUENCY = 1500   // 4500 RPM
const MAX_FILTER_FREQUENCY = 15000  // 45000 RPM
const FREQUENCY_PRECISION  = 0.01   // 1%


const fs = require('fs')
const argparse = require('argparse')

const lp2_create = require('./lib/lowpass2').create
const lp2_filter = require('./lib/lowpass2').filter
const hp2_create = require('./lib/highpass2').create
const hp2_filter = require('./lib/highpass2').filter


function generate_freq_list () {
  const result = []
  let freq = MAX_FILTER_FREQUENCY

  while (freq > MIN_FILTER_FREQUENCY) {
    result.unshift(freq)
    freq = Math.ceil(freq - freq * FREQUENCY_PRECISION)
  }

  return result
}


const cli = new argparse.ArgumentParser({ add_help: true })

cli.add_argument('file', {
  help: 'File to read',
  nargs: 1
})

const options = cli.parse_args()

const file = fs.readFileSync(options.file[0], { encoding: 'utf8' })
let data = file.split(/\r?\n/g).map(l => parseFloat(l.replace(',', '.')))


// Filter spikes
const lp_base = lp2_create(SAMPLE_FREQUENCY, 10000)
data = lp2_filter(data, lp_base)
/*
data = lp2_filter(data, lp_base)
data = lp2_filter(data, lp_base)
data = lp2_filter(data, lp_base)
data = lp2_filter(data, lp_base)

// Filter 50/60Hz
const hp_base = hp2_create(SAMPLE_FREQUENCY, 300)
data = hp2_filter(data, hp_base)
data = hp2_filter(data, hp_base)
data = hp2_filter(data, hp_base)
data = hp2_filter(data, hp_base)
data = hp2_filter(data, hp_base)
*/
const str_out = []
data.forEach(v => str_out.push(v.toFixed(4).replace('.', ',')))

fs.writeFileSync(`${options.file}.filtered`, str_out.join('\n'))

// write spectrum
const fft = require('@signalprocessing/transforms').fft
const fft_size = 16384

const fft_real = Array.from(data).slice(10000, 10000 + fft_size)
const [out_re, out_img] = fft(fft_real)

const energy = []
for (let i = 0; i < out_re.length; i++) {
  // energy.push(Math.sqrt(out_re[i] * out_re[i] + out_img[i] * out_img[i]))
  energy.push(Math.abs(out_re[i]))
}

str_out.length = 0;
energy.forEach(v => str_out.push(v.toFixed(4).replace('.', ',')))

fs.writeFileSync(`${options.file}.spectrum`, str_out.join('\n'))
