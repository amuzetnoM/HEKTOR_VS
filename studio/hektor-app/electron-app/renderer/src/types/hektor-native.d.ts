// TypeScript definitions for native addon
declare module 'hektor-native' {
  export interface QueryResult {
    id: string;
    distance: number;
    metadata?: Record<string, any>;
  }
  
  export interface DatabaseResult {
    success: boolean;
    path?: string;
    message: string;
  }
  
  export interface AddVectorResult {
    success: boolean;
    id: string;
    message: string;
  }
  
  export function openDatabase(path: string): DatabaseResult;
  export function closeDatabase(): DatabaseResult;
  export function addVector(vector: number[], metadata?: Record<string, any>): AddVectorResult;
  export function queryVectors(vector: number[], topK?: number): QueryResult[];
  export function queryVectorsAsync(
    vector: number[],
    topK: number,
    callback: (err: Error | null, results: QueryResult[]) => void
  ): void;
  export function getVersion(): string;
}
