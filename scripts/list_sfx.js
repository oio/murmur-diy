const fs = require('fs');
const path = require('path');

const dir = path.join(__dirname, '../simulation/sfx');
const files = fs.readdirSync(dir).filter(f => f.endsWith('.mp3') || f.endsWith('.wav'));

const lists = {
  city: [],
  forest: [], // might be 'field' in the code?
  water: [] // might be 'pond' in the code?
};

files.forEach(f => {
  if (f.startsWith('city')) lists.city.push(f);
  else if (f.startsWith('forest') || f.startsWith('squirrel')) lists.forest.push(f);
  else if (f.startsWith('water')) lists.water.push(f);
  else console.log("Unknown prefix:", f);
});

const outPath = path.join(__dirname, '../simulation/sfx_lists.js');
let content = '// Auto-generated list of SFX files\n';
content += 'const SFX_LISTS = ' + JSON.stringify(lists, null, 2) + ';\n';

fs.writeFileSync(outPath, content);
console.log("Wrote lists to simulation/sfx_lists.js");
console.log(`City: ${lists.city.length}, Forest: ${lists.forest.length}, Water: ${lists.water.length}`);
