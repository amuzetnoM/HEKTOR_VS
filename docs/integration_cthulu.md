# Integration: Vector Studio (Hektor) <-> Cthulu

This note documents recommended settings and patterns when using Vector Studio with Cthulu.

## Recommended UnifiedStore configuration
- Hybrid mode can be used to keep SQLite as a local cache and pgvector/remote for persistence.
- For Cthulu workloads, ensure write throughput by batching writes from `async_ingest_worker`.

## Schema compatibility
- Use the minimal `vector_schema` described in `models/cthulu/docs/vector_schema.md`.
- Ensure `payload` text and metadata fields are present.

## Example roundtrip
- See `examples/cthulu_roundtrip.py` for a minimal demonstration of creating a database, adding text, and searching.
