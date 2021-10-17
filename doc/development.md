Development
===========

## Extra components

You may need [USB isolator](https://www.aliexpress.com/af/usb-isolator.html) to
debug firmware.


## Rework for different power/components

### Current shunt

Existing 0.01R shunt is for ~ 200W motors @ 220v. For higher/lower power
change shunt value proportionally.


### MOSFET

Mosfet switch should work well at low control voltages (5.5v), and have low resistance.

### Protective diode

Diode, protecting MOSFET from self-induction voltage spikes should be ultra-fast
and have good power dissipation. Max current should be 2x of motor current.


### AC-DC

The best source for AD-DC tweaks is vendor's application manual. For LNK3204
that's [AN 70](https://ac-dc.power.com/sites/default/files/product-docs/an70.pdf).


#### input resistor(s)

When power on, input capacitor charge cause short, but very high pulse of power.
Ordinary resistors will fire from time to time. Use this kinds for robust work:

- Fusible.
- "High Pulse Withstanding" or "Anti-Surge" (SMD).
- MELF.

As quick hack (but not recommended) - try several ordinary 5% (not laser trimmed)
SMD resistors, mounted in vertical column.

Total value should be ~1K (as max as possible). Input cap should be 2.2uF (as
low as possible).


#### LNK3204 replacement

You can use LNK304/LNK306, but you should update feedback resistors
(according to chip datasheet) to make output 5.5 volts.


#### 100uF capacitor at step-down output

Must be low ESR. Very good choice are [Solid Polymer Electrolytic Capacitor](https://lcsc.com/products/Solid-Polymer-Electrolytic-Capacitor_927.html).


#### Inductor

Electronics average power consumption is ~ 25mA.

Any 1.5mH-3.3mH 100mA inductor will be fine. Cheap magnetic-resin shielded
inductor should be good choice for low EMI.

