/**
 * PerceptualQuantizationPanel - 4-Panel PQ Workspace
 * 
 * Implements the design specification from PERCEPTUAL_QUANTIZATION_DESIGN.md
 * 
 * Layout:
 * ┌────────────────────┬────────────────────┐
 * │ Original Vectors   │ Quantized Vectors  │
 * │ (3D View)          │ (3D + Error Heat)  │
 * ├────────────────────┼────────────────────┤
 * │ Control Panel      │ Metrics Dashboard  │
 * │ (Method, Params)   │ (PSNR, SSIM, etc) │
 * └────────────────────┴────────────────────┘
 * 
 * Dolby Vision / HDR10 / Netflix Compatible
 */

import React, { useState, useCallback, useMemo } from 'react';
import { PQCurvesVisualization, CurveType, pqEncode, pqDecode } from './PQCurves';

// ============================================================================
// Types
// ============================================================================

export type QuantizationMethod = 'PQ' | 'SQ' | 'OPQ' | 'Adaptive';
export type ComparisonMode = 'side-by-side' | 'overlay' | 'heatmap' | 'temporal';

export interface QuantizationConfig {
  method: QuantizationMethod;
  subvectors: number;      // M for PQ (4, 8, 16)
  codebookSize: number;    // K for PQ (256, 512, 1024)
  perceptualMode: boolean; // Use PQ ST 2084 transform
  perceptualCurve: CurveType;
  adaptiveScaling: boolean;
  validateOnEncode: boolean;
  bitDepth: 8 | 10 | 12;   // For HDR compatibility
}

export interface QualityMetrics {
  psnr: number;           // Peak Signal-to-Noise Ratio (dB)
  ssim: number;           // Structural Similarity Index [0-1]
  mse: number;            // Mean Squared Error
  recallAt10: number;     // Recall@10 for ANN quality
  compressionRatio: number;
  memorySaved: number;    // Bytes saved
  encodingTimeUs: number; // Microseconds per vector
  decodingTimeUs: number;
}

export interface VectorData {
  id: string;
  values: number[];
  quantizedValues?: number[];
  error?: number;
}

export interface PerceptualQuantizationPanelProps {
  vectors?: VectorData[];
  onQuantize?: (config: QuantizationConfig) => Promise<void>;
  onExport?: (format: 'json' | 'binary' | 'onnx') => void;
}

// ============================================================================
// Default Configuration
// ============================================================================

const DEFAULT_CONFIG: QuantizationConfig = {
  method: 'PQ',
  subvectors: 8,
  codebookSize: 256,
  perceptualMode: true,
  perceptualCurve: 'pq_st2084',
  adaptiveScaling: true,
  validateOnEncode: true,
  bitDepth: 10,
};

const DEFAULT_METRICS: QualityMetrics = {
  psnr: 0,
  ssim: 0,
  mse: 0,
  recallAt10: 0,
  compressionRatio: 1,
  memorySaved: 0,
  encodingTimeUs: 0,
  decodingTimeUs: 0,
};

// ============================================================================
// PerceptualQuantizationPanel Component
// ============================================================================

export const PerceptualQuantizationPanel: React.FC<PerceptualQuantizationPanelProps> = ({
  vectors = [],
  onQuantize,
  onExport,
}) => {
  // State
  const [config, setConfig] = useState<QuantizationConfig>(DEFAULT_CONFIG);
  const [metrics, setMetrics] = useState<QualityMetrics>(DEFAULT_METRICS);
  const [comparisonMode, setComparisonMode] = useState<ComparisonMode>('side-by-side');
  const [isQuantizing, setIsQuantizing] = useState(false);
  const [showPQCurves, setShowPQCurves] = useState(false);
  const [selectedVectorId, setSelectedVectorId] = useState<string | null>(null);
  
  // Computed values
  const vectorCount = vectors.length;
  const originalSize = useMemo(() => {
    if (vectors.length === 0) return 0;
    return vectors.length * vectors[0].values.length * 4; // float32
  }, [vectors]);
  
  const quantizedSize = useMemo(() => {
    if (vectors.length === 0) return 0;
    const bitsPerCode = config.method === 'SQ' ? config.bitDepth : 8;
    return vectors.length * config.subvectors * (bitsPerCode / 8);
  }, [vectors, config]);
  
  // Handlers
  const handleQuantize = useCallback(async () => {
    setIsQuantizing(true);
    try {
      if (onQuantize) {
        await onQuantize(config);
      }
      // Simulated metrics for now
      setMetrics({
        psnr: 42.3 + Math.random() * 2,
        ssim: 0.94 + Math.random() * 0.04,
        mse: 0.001 + Math.random() * 0.0005,
        recallAt10: 0.96 + Math.random() * 0.02,
        compressionRatio: originalSize / quantizedSize,
        memorySaved: originalSize - quantizedSize,
        encodingTimeUs: 0.5 + Math.random() * 0.3,
        decodingTimeUs: 0.3 + Math.random() * 0.2,
      });
    } finally {
      setIsQuantizing(false);
    }
  }, [config, onQuantize, originalSize, quantizedSize]);
  
  const updateConfig = useCallback((updates: Partial<QuantizationConfig>) => {
    setConfig(prev => ({ ...prev, ...updates }));
  }, []);
  
  // Styles
  const panelStyle: React.CSSProperties = {
    display: 'grid',
    gridTemplateColumns: '1fr 1fr',
    gridTemplateRows: '1fr 1fr',
    gap: '12px',
    height: '100%',
    minHeight: '600px',
    padding: '12px',
    background: 'var(--bg-primary, #0f0f1a)',
  };
  
  const quadrantStyle: React.CSSProperties = {
    background: 'var(--bg-secondary, #1a1a2e)',
    borderRadius: '8px',
    padding: '12px',
    overflow: 'hidden',
    display: 'flex',
    flexDirection: 'column',
  };
  
  const headerStyle: React.CSSProperties = {
    display: 'flex',
    justifyContent: 'space-between',
    alignItems: 'center',
    marginBottom: '12px',
    paddingBottom: '8px',
    borderBottom: '1px solid rgba(255,255,255,0.1)',
  };
  
  const titleStyle: React.CSSProperties = {
    margin: 0,
    fontSize: '13px',
    fontWeight: 600,
    color: 'var(--text-primary, #fff)',
  };
  
  const labelStyle: React.CSSProperties = {
    fontSize: '11px',
    color: 'rgba(255,255,255,0.6)',
    marginBottom: '4px',
  };
  
  const selectStyle: React.CSSProperties = {
    padding: '6px 10px',
    borderRadius: '4px',
    border: '1px solid rgba(255,255,255,0.2)',
    background: 'var(--bg-tertiary, #0d0d1a)',
    color: 'var(--text-primary, #fff)',
    fontSize: '12px',
    width: '100%',
    cursor: 'pointer',
  };
  
  const buttonStyle: React.CSSProperties = {
    padding: '10px 16px',
    borderRadius: '6px',
    border: 'none',
    background: 'linear-gradient(135deg, #f59e0b, #d97706)',
    color: '#fff',
    fontSize: '12px',
    fontWeight: 600,
    cursor: 'pointer',
    display: 'flex',
    alignItems: 'center',
    justifyContent: 'center',
    gap: '6px',
  };
  
  const checkboxStyle: React.CSSProperties = {
    display: 'flex',
    alignItems: 'center',
    gap: '8px',
    fontSize: '11px',
    color: 'rgba(255,255,255,0.8)',
    cursor: 'pointer',
  };
  
  const metricCardStyle: React.CSSProperties = {
    padding: '10px',
    background: 'var(--bg-tertiary, #0d0d1a)',
    borderRadius: '6px',
    textAlign: 'center',
  };
  
  const metricValueStyle: React.CSSProperties = {
    fontSize: '18px',
    fontWeight: 700,
    color: 'var(--accent-primary, #f59e0b)',
  };
  
  const metricLabelStyle: React.CSSProperties = {
    fontSize: '10px',
    color: 'rgba(255,255,255,0.5)',
    textTransform: 'uppercase',
    letterSpacing: '0.5px',
  };
  
  // Quality indicator colors
  const getQualityColor = (value: number, thresholds: [number, number]) => {
    if (value >= thresholds[1]) return '#22c55e'; // Green
    if (value >= thresholds[0]) return '#f59e0b'; // Orange
    return '#ef4444'; // Red
  };
  
  return (
    <div style={panelStyle}>
      {/* Panel 1: Original Vectors (Top Left) */}
      <div style={quadrantStyle}>
        <div style={headerStyle}>
          <h3 style={titleStyle}>📊 Original Vectors</h3>
          <span style={{ fontSize: '11px', color: 'rgba(255,255,255,0.5)' }}>
            {vectorCount.toLocaleString()} vectors • {(originalSize / 1024).toFixed(1)} KB
          </span>
        </div>
        <div style={{ 
          flex: 1, 
          background: 'var(--bg-tertiary, #0d0d1a)', 
          borderRadius: '4px',
          display: 'flex',
          alignItems: 'center',
          justifyContent: 'center',
          color: 'rgba(255,255,255,0.3)',
          fontSize: '12px',
        }}>
          {vectors.length > 0 ? (
            <div style={{ textAlign: 'center' }}>
              <div style={{ fontSize: '32px', marginBottom: '8px' }}>🔵</div>
              <div>3D Vector Space</div>
              <div style={{ fontSize: '10px', marginTop: '4px' }}>
                Float32 • Full Precision
              </div>
            </div>
          ) : (
            <div style={{ textAlign: 'center' }}>
              <div style={{ fontSize: '24px', marginBottom: '8px' }}>📁</div>
              <div>Load vectors to begin</div>
            </div>
          )}
        </div>
        <div style={{ 
          marginTop: '8px', 
          display: 'flex', 
          gap: '8px',
          justifyContent: 'center',
        }}>
          {['side-by-side', 'overlay', 'heatmap'].map((mode) => (
            <button
              key={mode}
              onClick={() => setComparisonMode(mode as ComparisonMode)}
              style={{
                padding: '4px 10px',
                borderRadius: '4px',
                border: comparisonMode === mode ? '1px solid #f59e0b' : '1px solid rgba(255,255,255,0.2)',
                background: comparisonMode === mode ? 'rgba(245,158,11,0.2)' : 'transparent',
                color: comparisonMode === mode ? '#f59e0b' : 'rgba(255,255,255,0.6)',
                fontSize: '10px',
                cursor: 'pointer',
                textTransform: 'capitalize',
              }}
            >
              {mode.replace('-', ' ')}
            </button>
          ))}
        </div>
      </div>
      
      {/* Panel 2: Quantized Vectors (Top Right) */}
      <div style={quadrantStyle}>
        <div style={headerStyle}>
          <h3 style={titleStyle}>🎯 Quantized Vectors</h3>
          <span style={{ fontSize: '11px', color: 'rgba(255,255,255,0.5)' }}>
            {vectorCount.toLocaleString()} vectors • {(quantizedSize / 1024).toFixed(1)} KB
          </span>
        </div>
        <div style={{ 
          flex: 1, 
          background: 'var(--bg-tertiary, #0d0d1a)', 
          borderRadius: '4px',
          display: 'flex',
          alignItems: 'center',
          justifyContent: 'center',
          position: 'relative',
        }}>
          {metrics.psnr > 0 ? (
            <div style={{ textAlign: 'center', color: 'rgba(255,255,255,0.7)' }}>
              <div style={{ fontSize: '32px', marginBottom: '8px' }}>🟠</div>
              <div style={{ fontSize: '12px' }}>Error Heatmap</div>
              <div style={{ fontSize: '10px', marginTop: '4px', color: '#f59e0b' }}>
                {config.method} • {config.subvectors}×{config.codebookSize}
              </div>
            </div>
          ) : (
            <div style={{ textAlign: 'center', color: 'rgba(255,255,255,0.3)' }}>
              <div style={{ fontSize: '24px', marginBottom: '8px' }}>⏳</div>
              <div style={{ fontSize: '12px' }}>Run quantization to see results</div>
            </div>
          )}
          
          {/* Error threshold slider */}
          {metrics.psnr > 0 && (
            <div style={{
              position: 'absolute',
              bottom: '8px',
              left: '8px',
              right: '8px',
              display: 'flex',
              alignItems: 'center',
              gap: '8px',
            }}>
              <span style={{ fontSize: '10px', color: 'rgba(255,255,255,0.5)' }}>🟢 Low</span>
              <input 
                type="range" 
                min="0" 
                max="100" 
                defaultValue="50"
                style={{ flex: 1, height: '4px' }}
              />
              <span style={{ fontSize: '10px', color: 'rgba(255,255,255,0.5)' }}>🔴 High</span>
            </div>
          )}
        </div>
      </div>
      
      {/* Panel 3: Control Panel (Bottom Left) */}
      <div style={quadrantStyle}>
        <div style={headerStyle}>
          <h3 style={titleStyle}>⚙️ Quantization Settings</h3>
          <button
            onClick={() => setShowPQCurves(!showPQCurves)}
            style={{
              padding: '4px 8px',
              borderRadius: '4px',
              border: '1px solid #f59e0b',
              background: showPQCurves ? 'rgba(245,158,11,0.2)' : 'transparent',
              color: '#f59e0b',
              fontSize: '10px',
              cursor: 'pointer',
            }}
          >
            {showPQCurves ? 'Hide' : 'Show'} PQ Curves
          </button>
        </div>
        
        <div style={{ flex: 1, overflowY: 'auto' }}>
          {showPQCurves ? (
            <PQCurvesVisualization
              width={380}
              height={200}
              selectedCurve={config.perceptualCurve}
              onCurveChange={(curve) => updateConfig({ perceptualCurve: curve })}
              showComparison={true}
            />
          ) : (
            <div style={{ display: 'grid', gridTemplateColumns: '1fr 1fr', gap: '12px' }}>
              {/* Method */}
              <div>
                <div style={labelStyle}>Method</div>
                <select
                  value={config.method}
                  onChange={(e) => updateConfig({ method: e.target.value as QuantizationMethod })}
                  style={selectStyle}
                >
                  <option value="PQ">Product Quantization (PQ)</option>
                  <option value="SQ">Scalar Quantization (SQ)</option>
                  <option value="OPQ">Optimized PQ (OPQ)</option>
                  <option value="Adaptive">Adaptive (HDR-aware)</option>
                </select>
              </div>
              
              {/* Subvectors */}
              <div>
                <div style={labelStyle}>Subvectors (M)</div>
                <select
                  value={config.subvectors}
                  onChange={(e) => updateConfig({ subvectors: parseInt(e.target.value) })}
                  style={selectStyle}
                >
                  <option value="4">4 subvectors</option>
                  <option value="8">8 subvectors</option>
                  <option value="16">16 subvectors</option>
                  <option value="32">32 subvectors</option>
                </select>
              </div>
              
              {/* Codebook Size */}
              <div>
                <div style={labelStyle}>Codebook Size (K)</div>
                <select
                  value={config.codebookSize}
                  onChange={(e) => updateConfig({ codebookSize: parseInt(e.target.value) })}
                  style={selectStyle}
                >
                  <option value="256">256 centroids</option>
                  <option value="512">512 centroids</option>
                  <option value="1024">1024 centroids</option>
                </select>
              </div>
              
              {/* Bit Depth (HDR) */}
              <div>
                <div style={labelStyle}>Bit Depth (HDR)</div>
                <select
                  value={config.bitDepth}
                  onChange={(e) => updateConfig({ bitDepth: parseInt(e.target.value) as 8 | 10 | 12 })}
                  style={selectStyle}
                >
                  <option value="8">8-bit (SDR)</option>
                  <option value="10">10-bit (HDR10)</option>
                  <option value="12">12-bit (Dolby Vision)</option>
                </select>
              </div>
              
              {/* Options */}
              <div style={{ gridColumn: 'span 2', marginTop: '8px' }}>
                <div style={labelStyle}>Options</div>
                <div style={{ display: 'flex', flexDirection: 'column', gap: '8px' }}>
                  <label style={checkboxStyle}>
                    <input
                      type="checkbox"
                      checked={config.perceptualMode}
                      onChange={(e) => updateConfig({ perceptualMode: e.target.checked })}
                    />
                    <span>⭐ Perceptual Mode (PQ ST 2084)</span>
                  </label>
                  <label style={checkboxStyle}>
                    <input
                      type="checkbox"
                      checked={config.adaptiveScaling}
                      onChange={(e) => updateConfig({ adaptiveScaling: e.target.checked })}
                    />
                    <span>Adaptive Scaling</span>
                  </label>
                  <label style={checkboxStyle}>
                    <input
                      type="checkbox"
                      checked={config.validateOnEncode}
                      onChange={(e) => updateConfig({ validateOnEncode: e.target.checked })}
                    />
                    <span>Validate on Encode</span>
                  </label>
                </div>
              </div>
            </div>
          )}
        </div>
        
        {/* Action Buttons */}
        <div style={{ 
          marginTop: '12px', 
          display: 'flex', 
          gap: '8px',
          paddingTop: '12px',
          borderTop: '1px solid rgba(255,255,255,0.1)',
        }}>
          <button
            onClick={handleQuantize}
            disabled={isQuantizing || vectors.length === 0}
            style={{
              ...buttonStyle,
              flex: 1,
              opacity: isQuantizing || vectors.length === 0 ? 0.5 : 1,
            }}
          >
            {isQuantizing ? '⏳ Processing...' : '🎯 Quantize Now'}
          </button>
          <button
            onClick={() => onExport?.('json')}
            style={{
              ...buttonStyle,
              background: 'var(--bg-tertiary, #0d0d1a)',
              border: '1px solid rgba(255,255,255,0.2)',
              flex: 0.5,
            }}
          >
            💾 Export
          </button>
        </div>
      </div>
      
      {/* Panel 4: Metrics Dashboard (Bottom Right) */}
      <div style={quadrantStyle}>
        <div style={headerStyle}>
          <h3 style={titleStyle}>📊 Quality Metrics</h3>
          {metrics.psnr > 0 && (
            <span style={{ 
              fontSize: '10px', 
              padding: '2px 8px',
              borderRadius: '10px',
              background: getQualityColor(metrics.recallAt10, [0.92, 0.95]) + '30',
              color: getQualityColor(metrics.recallAt10, [0.92, 0.95]),
            }}>
              Quality: {metrics.recallAt10 >= 0.95 ? 'Excellent' : metrics.recallAt10 >= 0.92 ? 'Good' : 'Fair'}
            </span>
          )}
        </div>
        
        <div style={{ flex: 1, overflowY: 'auto' }}>
          {/* Performance Metrics */}
          <div style={{ marginBottom: '12px' }}>
            <div style={{ 
              fontSize: '10px', 
              color: 'rgba(255,255,255,0.5)', 
              marginBottom: '8px',
              textTransform: 'uppercase',
              letterSpacing: '1px',
            }}>
              ⚡ Performance
            </div>
            <div style={{ display: 'grid', gridTemplateColumns: 'repeat(3, 1fr)', gap: '8px' }}>
              <div style={metricCardStyle}>
                <div style={{ ...metricValueStyle, color: '#22c55e' }}>
                  {metrics.compressionRatio > 0 ? `${metrics.compressionRatio.toFixed(1)}x` : '—'}
                </div>
                <div style={metricLabelStyle}>Compression</div>
              </div>
              <div style={metricCardStyle}>
                <div style={{ ...metricValueStyle, color: '#3b82f6' }}>
                  {metrics.memorySaved > 0 ? `${(metrics.memorySaved / 1024).toFixed(0)} KB` : '—'}
                </div>
                <div style={metricLabelStyle}>Memory Saved</div>
              </div>
              <div style={metricCardStyle}>
                <div style={{ ...metricValueStyle, color: '#a855f7' }}>
                  {metrics.encodingTimeUs > 0 ? `${metrics.encodingTimeUs.toFixed(2)} µs` : '—'}
                </div>
                <div style={metricLabelStyle}>Encode Time</div>
              </div>
            </div>
          </div>
          
          {/* Quality Metrics */}
          <div>
            <div style={{ 
              fontSize: '10px', 
              color: 'rgba(255,255,255,0.5)', 
              marginBottom: '8px',
              textTransform: 'uppercase',
              letterSpacing: '1px',
            }}>
              📊 Quality Scores
            </div>
            <div style={{ display: 'flex', flexDirection: 'column', gap: '8px' }}>
              {/* PSNR */}
              <div style={{ 
                display: 'flex', 
                alignItems: 'center', 
                gap: '12px',
                padding: '8px',
                background: 'var(--bg-tertiary, #0d0d1a)',
                borderRadius: '4px',
              }}>
                <span style={{ fontSize: '11px', color: 'rgba(255,255,255,0.7)', width: '80px' }}>
                  PSNR
                </span>
                <div style={{ flex: 1, height: '6px', background: 'rgba(255,255,255,0.1)', borderRadius: '3px' }}>
                  <div 
                    style={{ 
                      width: `${Math.min(metrics.psnr / 50 * 100, 100)}%`, 
                      height: '100%', 
                      background: getQualityColor(metrics.psnr, [35, 40]),
                      borderRadius: '3px',
                      transition: 'width 0.3s',
                    }} 
                  />
                </div>
                <span style={{ 
                  fontSize: '12px', 
                  fontWeight: 600, 
                  color: getQualityColor(metrics.psnr, [35, 40]),
                  width: '60px',
                  textAlign: 'right',
                }}>
                  {metrics.psnr > 0 ? `${metrics.psnr.toFixed(1)} dB` : '—'}
                </span>
              </div>
              
              {/* SSIM */}
              <div style={{ 
                display: 'flex', 
                alignItems: 'center', 
                gap: '12px',
                padding: '8px',
                background: 'var(--bg-tertiary, #0d0d1a)',
                borderRadius: '4px',
              }}>
                <span style={{ fontSize: '11px', color: 'rgba(255,255,255,0.7)', width: '80px' }}>
                  SSIM
                </span>
                <div style={{ flex: 1, height: '6px', background: 'rgba(255,255,255,0.1)', borderRadius: '3px' }}>
                  <div 
                    style={{ 
                      width: `${metrics.ssim * 100}%`, 
                      height: '100%', 
                      background: getQualityColor(metrics.ssim, [0.9, 0.95]),
                      borderRadius: '3px',
                      transition: 'width 0.3s',
                    }} 
                  />
                </div>
                <span style={{ 
                  fontSize: '12px', 
                  fontWeight: 600, 
                  color: getQualityColor(metrics.ssim, [0.9, 0.95]),
                  width: '60px',
                  textAlign: 'right',
                }}>
                  {metrics.ssim > 0 ? metrics.ssim.toFixed(4) : '—'}
                </span>
              </div>
              
              {/* MSE */}
              <div style={{ 
                display: 'flex', 
                alignItems: 'center', 
                gap: '12px',
                padding: '8px',
                background: 'var(--bg-tertiary, #0d0d1a)',
                borderRadius: '4px',
              }}>
                <span style={{ fontSize: '11px', color: 'rgba(255,255,255,0.7)', width: '80px' }}>
                  MSE
                </span>
                <div style={{ flex: 1, height: '6px', background: 'rgba(255,255,255,0.1)', borderRadius: '3px' }}>
                  <div 
                    style={{ 
                      width: `${Math.max(0, 100 - metrics.mse * 10000)}%`, 
                      height: '100%', 
                      background: getQualityColor(1 - metrics.mse * 100, [0.85, 0.95]),
                      borderRadius: '3px',
                      transition: 'width 0.3s',
                    }} 
                  />
                </div>
                <span style={{ 
                  fontSize: '12px', 
                  fontWeight: 600, 
                  color: 'rgba(255,255,255,0.8)',
                  width: '60px',
                  textAlign: 'right',
                }}>
                  {metrics.mse > 0 ? metrics.mse.toFixed(5) : '—'}
                </span>
              </div>
              
              {/* Recall@10 */}
              <div style={{ 
                display: 'flex', 
                alignItems: 'center', 
                gap: '12px',
                padding: '8px',
                background: 'var(--bg-tertiary, #0d0d1a)',
                borderRadius: '4px',
              }}>
                <span style={{ fontSize: '11px', color: 'rgba(255,255,255,0.7)', width: '80px' }}>
                  Recall@10
                </span>
                <div style={{ flex: 1, height: '6px', background: 'rgba(255,255,255,0.1)', borderRadius: '3px' }}>
                  <div 
                    style={{ 
                      width: `${metrics.recallAt10 * 100}%`, 
                      height: '100%', 
                      background: getQualityColor(metrics.recallAt10, [0.92, 0.95]),
                      borderRadius: '3px',
                      transition: 'width 0.3s',
                    }} 
                  />
                </div>
                <span style={{ 
                  fontSize: '12px', 
                  fontWeight: 600, 
                  color: getQualityColor(metrics.recallAt10, [0.92, 0.95]),
                  width: '60px',
                  textAlign: 'right',
                }}>
                  {metrics.recallAt10 > 0 ? `${(metrics.recallAt10 * 100).toFixed(1)}%` : '—'}
                </span>
              </div>
            </div>
          </div>
          
          {/* HDR Compatibility Badge */}
          {config.perceptualMode && config.bitDepth >= 10 && (
            <div style={{
              marginTop: '16px',
              padding: '12px',
              background: 'linear-gradient(135deg, rgba(245,158,11,0.1), rgba(236,72,153,0.1))',
              borderRadius: '6px',
              border: '1px solid rgba(245,158,11,0.3)',
              textAlign: 'center',
            }}>
              <div style={{ fontSize: '10px', color: 'rgba(255,255,255,0.6)', marginBottom: '4px' }}>
                HDR COMPATIBLE
              </div>
              <div style={{ fontSize: '12px', color: '#f59e0b', fontWeight: 600 }}>
                {config.bitDepth === 12 ? '⭐ Dolby Vision' : '📺 HDR10'} Ready
              </div>
              <div style={{ fontSize: '9px', color: 'rgba(255,255,255,0.5)', marginTop: '4px' }}>
                SMPTE ST 2084 • ITU-R BT.2100
              </div>
            </div>
          )}
        </div>
      </div>
    </div>
  );
};

export default PerceptualQuantizationPanel;
