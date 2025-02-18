npm run clean              # Clean old build files (if defined in package.json)
npm version patch          # Increment the patch version (e.g., 1.0.0 → 1.0.1)
# npm run prepublish
npm run build:release      # Build the package for release (defined in package.json)
# npm publish --access public  # Publish to npm (for scoped packages, requires --access public)
echo "Published successfully"  # Confirmation message
