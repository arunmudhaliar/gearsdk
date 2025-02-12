// NOTE: This script is no more used in build process.
const fs = require("fs");
const os = require("os");

const package_template_json_path = "./package-template.json";
const package_template_data = JSON.parse(fs.readFileSync(package_template_json_path, "utf-8"));

const platform = os.platform(); // "darwin" for macOS, "linux" for Linux

if (platform === "darwin") {
    package_template_data.name = "@amudaliar/gsdk-serverplugin-mac";
} else if (platform === "linux") {
    package_template_data.name = "@amudaliar/gsdk-serverplugin-linux";
} else {
    console.error(`Unsupported OS: ${platform}`);
    process.exit(1);
}

// Save the modified package.json
const package_json_path = "./package.json";
fs.writeFileSync(package_json_path, JSON.stringify(package_template_data, null, 2) + "\n");

console.log(`Updated package name to: ${package_template_data.name}`);
