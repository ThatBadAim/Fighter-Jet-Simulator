# Agent Guidelines & Rules

## Mandatory Graphify First Step

This project maintains an AST-based knowledge graph in `graphify-out/`.

**CRITICAL RULE FOR ALL AI AGENTS:**
Any time any agent works on, explores, or modifies this project, the agent **MUST ALWAYS** query the graphify knowledge graph **FIRST** before performing ad-hoc searches, reading raw files, or generating plans.

1. **Always Query the Graph First**:
   When `graphify-out/graph.json` exists, execute:
   ```bash
   graphify query "<question or task keywords>"
   ```
   (or use `query_graph` if MCP is enabled).

2. **Relationships & Path Tracing**:
   Use `graphify path "<nodeA>" "<nodeB>"` (or `shortest_path`) to inspect dependencies, call graphs, and architectural couplings.

3. **Symbol & Concept Deep Dives**:
   Use `graphify explain "<concept>"` (or `get_node`) to inspect specific classes, structs, or functions with their direct connections.

4. **Wiki Navigation**:
   If `graphify-out/wiki/index.md` exists, navigate the generated knowledge wiki rather than inspecting raw source files blindly.

5. **Graph Synchronization**:
   After creating, modifying, or deleting any code files in the session, always run:
   ```bash
   graphify update .
   ```
   to keep the knowledge graph current (AST-only, instant, zero API cost).
