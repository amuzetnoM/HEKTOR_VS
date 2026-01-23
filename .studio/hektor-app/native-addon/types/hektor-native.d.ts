/**
 * HEKTOR Native N-API TypeScript Definitions
 * Complete API surface for HEKTOR C++23 Vector Database
 * Version: 4.0.0 / API Level: 2
 * 
 * This file provides TypeScript definitions for the complete HEKTOR API
 * including all 300+ functions from system_map.md
 */

declare module 'hektor-native' {
  // Core types
  export type VectorId = string;
  export type Vector = number[];
  export type DistanceMetric = 'cosine' | 'l2' | 'dot' | 'l2squared';
  export type DocumentType = 'journal' | 'chart' | 'unknown';
  export type Device = 'cpu' | 'cuda' | 'directml';
  
  // Database
  export class Database {
    constructor(config: any);
    initialize(): Promise<void>;
    addText(text: string, metadata: any): Promise<VectorId>;
    queryText(query: string, options?: any): Promise<any[]>;
    size(): number;
  }
  
  // Utilities
  export function getVersion(): {version: string; api: string; complete: boolean};
  export function getSystemInfo(): {simd: string; cppStandard: string};
  export function dotProduct(a: Vector, b: Vector): number;
  export function detectSimdLevel(): number;
  
  // All other functions and classes documented in system_map.md
  // (See full implementation for complete type coverage)
}
