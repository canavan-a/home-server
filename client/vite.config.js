import { defineConfig } from "vite";
import react from "@vitejs/plugin-react";
import { createHash } from "crypto";

const buildHash = createHash("md5").update(Date.now().toString()).digest("hex");

// https://vitejs.dev/config/
export default defineConfig({
  plugins: [react()],
  define: {
    // Inject build hash into `import.meta.env.BUILD_HASH`
    "import.meta.env.BUILD_HASH": JSON.stringify(buildHash),
  },
});
