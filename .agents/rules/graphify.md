---
trigger: always_on
description: Mandatory: Any agent using this project MUST query the graphify knowledge graph first before reading files or making changes.
---

## Mandatory Graphify Knowledge Graph Rule

This project maintains a graphify knowledge graph at `graphify-out/`.

**MANDATORY FIRST STEP FOR ALL AGENTS:**
Any time any agent uses or works on this project, the agent **MUST ALWAYS** query the graphify knowledge graph **FIRST** before performing ad-hoc grep searches, reading raw files, or proposing architecture changes:

1. **Always Query First**: When `graphify-out/graph.json` exists, run:
   ```bash
   graphify query "<question or task keywords>"
   ```
   (or use the `query_graph` MCP tool).
2. **Relationship Analysis**: Use `graphify path "<A>" "<B>"` (or `shortest_path`) for module/class connections and dependency paths.
3. **Concept Deep-Dive**: Use `graphify explain "<concept>"` (or `get_node`) for focused component/symbol inspection.
4. **Wiki Navigation**: If `graphify-out/wiki/index.md` exists, navigate the wiki structure instead of doing unguided file reads.
5. **Graph Synchronization**: After modifying or creating any code files in a session, immediately run:
   ```bash
   graphify update .
   ```
   to keep the AST knowledge graph up to date.

