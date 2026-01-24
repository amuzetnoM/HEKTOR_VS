/**
 * PQ Curves Visualization Component
 * 
 * Implements SMPTE ST 2084 (Dolby PQ) and HLG (Rec.2100) transfer functions
 * for perceptual quantization of vector embeddings.
 * 
 * Dolby Vision / HDR10 compatible - Netflix/Disney+ encoding ready
 */

import React, { useMemo, useCallback } from 'react';

// ============================================================================
// SMPTE ST 2084 (PQ) Constants - Exact Dolby Specification
// ============================================================================

const PQ_CONSTANTS = {
  M1: 2610.0 / 16384.0,           // 0.1593017578125
  M2: (2523.0 / 4096.0) * 128.0,  // 78.84375
  C1: 3424.0 / 4096.0,            // 0.8359375
  C2: (2413.0 / 4096.0) * 32.0,   // 18.8515625
  C3: (2392.0 / 4096.0) * 32.0,   // 18.6875
  PEAK_LUMINANCE: 10000.0,        // 10,000 nits (HDR peak)
};

const HLG_CONSTANTS = {
  A: 0.17883277,
  B: 0.28466892,  // 1 - 4*A
  C: 0.55991073,  // 0.5 - A * ln(4*A)
};

// ============================================================================
// PQ Curve Functions (SMPTE ST 2084 / Dolby Vision)
// ============================================================================

/**
 * Encode linear luminance to PQ (perceptual) space
 * Input: Linear light value [0, 10000] nits
 * Output: PQ-encoded value [0, 1]
 */
export function pqEncode(linearLuminance: number): number {
  const { M1, M2, C1, C2, C3, PEAK_LUMINANCE } = PQ_CONSTANTS;
  
  // Normalize to [0, 1]
  const L = Math.max(0, Math.min(linearLuminance / PEAK_LUMINANCE, 1));
  
  if (L <= 0) return 0;
  if (L >= 1) return 1;
  
  // PQ EOTF encoding
  const Lm1 = Math.pow(L, M1);
  const numerator = C1 + C2 * Lm1;
  const denominator = 1 + C3 * Lm1;
  const N = numerator / denominator;
  
  return Math.pow(N, M2);
}

/**
 * Decode PQ (perceptual) space to linear luminance
 * Input: PQ-encoded value [0, 1]
 * Output: Linear luminance [0, 10000] nits
 */
export function pqDecode(pqValue: number): number {
  const { M1, M2, C1, C2, C3, PEAK_LUMINANCE } = PQ_CONSTANTS;
  
  if (pqValue <= 0) return 0;
  if (pqValue >= 1) return PEAK_LUMINANCE;
  
  // PQ OETF decoding (inverse)
  const Nm2 = Math.pow(pqValue, 1 / M2);
  const numerator = Math.max(Nm2 - C1, 0);
  const denominator = C2 - C3 * Nm2;
  
  if (denominator <= 0) return PEAK_LUMINANCE;
  
  const L = Math.pow(numerator / denominator, 1 / M1);
  
  return L * PEAK_LUMINANCE;
}

// ============================================================================
// HLG Curve Functions (Rec. 2100 - BBC/NHK)
// ============================================================================

/**
 * Encode linear scene luminance to HLG
 * Input: Linear scene light [0, 1]
 * Output: HLG-encoded value [0, 1]
 */
export function hlgEncode(linearValue: number): number {
  const { A, B, C } = HLG_CONSTANTS;
  
  if (linearValue <= 0) return 0;
  if (linearValue >= 1) return 1;
  
  if (linearValue <= 1 / 12) {
    // Linear part (shadow region)
    return Math.sqrt(3 * linearValue);
  } else {
    // Logarithmic part (highlight region)
    return A * Math.log(12 * linearValue - B) + C;
  }
}

/**
 * Decode HLG to linear scene luminance
 * Input: HLG-encoded value [0, 1]
 * Output: Linear scene light [0, 1]
 */
export function hlgDecode(hlgValue: number): number {
  const { A, B, C } = HLG_CONSTANTS;
  
  if (hlgValue <= 0) return 0;
  if (hlgValue >= 1) return 1;
  
  if (hlgValue <= 0.5) {
    // Linear part inverse
    return (hlgValue * hlgValue) / 3;
  } else {
    // Logarithmic part inverse
    return (Math.exp((hlgValue - C) / A) + B) / 12;
  }
}

// ============================================================================
// Gamma Curve Functions (sRGB/Rec.709/DCI-P3)
// ============================================================================

export function gammaEncode(linearValue: number, gamma: number = 2.2): number {
  if (linearValue <= 0) return 0;
  if (linearValue >= 1) return 1;
  return Math.pow(linearValue, 1 / gamma);
}

export function gammaDecode(gammaValue: number, gamma: number = 2.2): number {
  if (gammaValue <= 0) return 0;
  if (gammaValue >= 1) return 1;
  return Math.pow(gammaValue, gamma);
}

// ============================================================================
// PQ Curves Types
// ============================================================================

export type CurveType = 'linear' | 'gamma22' | 'gamma24' | 'pq_st2084' | 'hlg_rec2100';

export interface CurvePoint {
  x: number;  // Input value
  y: number;  // Output value (encoded)
}

export interface PQCurvesProps {
  width?: number;
  height?: number;
  selectedCurve?: CurveType;
  onCurveChange?: (curve: CurveType) => void;
  showComparison?: boolean;
  interactiveMode?: boolean;
}

// ============================================================================
// PQ Curves Visualization Component
// ============================================================================

export const PQCurvesVisualization: React.FC<PQCurvesProps> = ({
  width = 600,
  height = 400,
  selectedCurve = 'pq_st2084',
  onCurveChange,
  showComparison = true,
  interactiveMode = true,
}) => {
  // Generate curve data points
  const curveData = useMemo(() => {
    const points: Record<CurveType, CurvePoint[]> = {
      linear: [],
      gamma22: [],
      gamma24: [],
      pq_st2084: [],
      hlg_rec2100: [],
    };
    
    const numPoints = 256;
    
    for (let i = 0; i <= numPoints; i++) {
      const x = i / numPoints;
      
      points.linear.push({ x, y: x });
      points.gamma22.push({ x, y: gammaEncode(x, 2.2) });
      points.gamma24.push({ x, y: gammaEncode(x, 2.4) });
      points.pq_st2084.push({ x, y: pqEncode(x * 10000) }); // Scale to nits
      points.hlg_rec2100.push({ x, y: hlgEncode(x) });
    }
    
    return points;
  }, []);
  
  // SVG path generator
  const generatePath = useCallback((points: CurvePoint[]): string => {
    if (points.length === 0) return '';
    
    const paddingX = 60;
    const paddingY = 40;
    const graphWidth = width - paddingX * 2;
    const graphHeight = height - paddingY * 2;
    
    const path = points.map((point, i) => {
      const x = paddingX + point.x * graphWidth;
      const y = height - paddingY - point.y * graphHeight;
      return `${i === 0 ? 'M' : 'L'} ${x.toFixed(2)} ${y.toFixed(2)}`;
    });
    
    return path.join(' ');
  }, [width, height]);
  
  // Curve colors
  const curveColors: Record<CurveType, string> = {
    linear: '#6b7280',
    gamma22: '#22c55e',
    gamma24: '#3b82f6',
    pq_st2084: '#f59e0b',   // Dolby PQ - Orange/Gold (official-ish)
    hlg_rec2100: '#ec4899', // HLG - Pink
  };
  
  const curveLabels: Record<CurveType, string> = {
    linear: 'Linear (No Transform)',
    gamma22: 'Gamma 2.2 (sRGB/Rec.709)',
    gamma24: 'Gamma 2.4 (DCI-P3)',
    pq_st2084: 'PQ ST 2084 (Dolby Vision/HDR10)',
    hlg_rec2100: 'HLG Rec.2100 (BBC/NHK)',
  };
  
  const paddingX = 60;
  const paddingY = 40;
  
  return (
    <div className="pq-curves-container" style={{ 
      background: 'var(--bg-secondary, #1a1a2e)',
      borderRadius: '8px',
      padding: '16px',
    }}>
      <h3 style={{ 
        margin: '0 0 16px 0', 
        color: 'var(--text-primary, #fff)',
        fontSize: '14px',
        fontWeight: 600,
      }}>
        Perceptual Transfer Functions (EOTF/OETF)
      </h3>
      
      {/* Curve Type Selector */}
      <div style={{ 
        display: 'flex', 
        gap: '8px', 
        marginBottom: '16px',
        flexWrap: 'wrap',
      }}>
        {(Object.keys(curveLabels) as CurveType[]).map((curve) => (
          <button
            key={curve}
            onClick={() => onCurveChange?.(curve)}
            style={{
              padding: '6px 12px',
              borderRadius: '4px',
              border: selectedCurve === curve ? '2px solid' : '1px solid',
              borderColor: curveColors[curve],
              background: selectedCurve === curve ? curveColors[curve] + '30' : 'transparent',
              color: curveColors[curve],
              cursor: 'pointer',
              fontSize: '11px',
              fontWeight: selectedCurve === curve ? 600 : 400,
              transition: 'all 0.2s',
            }}
          >
            {curve === 'pq_st2084' ? '⭐ PQ ST 2084' : 
             curve === 'hlg_rec2100' ? 'HLG Rec.2100' : 
             curveLabels[curve].split(' ')[0]}
          </button>
        ))}
      </div>
      
      {/* SVG Visualization */}
      <svg 
        width={width} 
        height={height} 
        style={{ 
          background: 'var(--bg-tertiary, #0d0d1a)',
          borderRadius: '4px',
        }}
      >
        {/* Grid */}
        {[0, 0.25, 0.5, 0.75, 1].map((val) => (
          <React.Fragment key={val}>
            {/* Horizontal grid lines */}
            <line
              x1={paddingX}
              y1={height - paddingY - val * (height - paddingY * 2)}
              x2={width - paddingX}
              y2={height - paddingY - val * (height - paddingY * 2)}
              stroke="rgba(255,255,255,0.1)"
              strokeDasharray="4"
            />
            {/* Vertical grid lines */}
            <line
              x1={paddingX + val * (width - paddingX * 2)}
              y1={paddingY}
              x2={paddingX + val * (width - paddingX * 2)}
              y2={height - paddingY}
              stroke="rgba(255,255,255,0.1)"
              strokeDasharray="4"
            />
            {/* Y-axis labels */}
            <text
              x={paddingX - 10}
              y={height - paddingY - val * (height - paddingY * 2) + 4}
              fill="rgba(255,255,255,0.5)"
              fontSize="10"
              textAnchor="end"
            >
              {(val * 100).toFixed(0)}%
            </text>
            {/* X-axis labels */}
            <text
              x={paddingX + val * (width - paddingX * 2)}
              y={height - paddingY + 16}
              fill="rgba(255,255,255,0.5)"
              fontSize="10"
              textAnchor="middle"
            >
              {selectedCurve === 'pq_st2084' ? `${(val * 10000).toFixed(0)}` : (val * 100).toFixed(0) + '%'}
            </text>
          </React.Fragment>
        ))}
        
        {/* Axis labels */}
        <text
          x={width / 2}
          y={height - 8}
          fill="rgba(255,255,255,0.7)"
          fontSize="11"
          textAnchor="middle"
        >
          {selectedCurve === 'pq_st2084' ? 'Linear Luminance (nits)' : 'Linear Input'}
        </text>
        <text
          x={14}
          y={height / 2}
          fill="rgba(255,255,255,0.7)"
          fontSize="11"
          textAnchor="middle"
          transform={`rotate(-90, 14, ${height / 2})`}
        >
          Encoded Output
        </text>
        
        {/* Draw comparison curves (faded) */}
        {showComparison && (Object.keys(curveData) as CurveType[]).map((curve) => (
          curve !== selectedCurve && (
            <path
              key={curve}
              d={generatePath(curveData[curve])}
              fill="none"
              stroke={curveColors[curve]}
              strokeWidth="1"
              opacity="0.2"
            />
          )
        ))}
        
        {/* Draw selected curve (highlighted) */}
        <path
          d={generatePath(curveData[selectedCurve])}
          fill="none"
          stroke={curveColors[selectedCurve]}
          strokeWidth="3"
          strokeLinecap="round"
          filter="url(#glow)"
        />
        
        {/* Glow filter */}
        <defs>
          <filter id="glow" x="-50%" y="-50%" width="200%" height="200%">
            <feGaussianBlur stdDeviation="2" result="coloredBlur"/>
            <feMerge>
              <feMergeNode in="coloredBlur"/>
              <feMergeNode in="SourceGraphic"/>
            </feMerge>
          </filter>
        </defs>
      </svg>
      
      {/* Info Panel */}
      <div style={{
        marginTop: '16px',
        padding: '12px',
        background: 'var(--bg-tertiary, #0d0d1a)',
        borderRadius: '4px',
        borderLeft: `3px solid ${curveColors[selectedCurve]}`,
      }}>
        <h4 style={{ 
          margin: '0 0 8px 0', 
          color: curveColors[selectedCurve],
          fontSize: '13px',
        }}>
          {curveLabels[selectedCurve]}
        </h4>
        <p style={{ 
          margin: 0, 
          color: 'rgba(255,255,255,0.7)',
          fontSize: '11px',
          lineHeight: 1.5,
        }}>
          {selectedCurve === 'pq_st2084' && (
            <>
              <strong>SMPTE ST 2084</strong> - The Perceptual Quantizer used in 
              <strong> Dolby Vision</strong> and <strong>HDR10</strong>. 
              Optimized for 10,000 nit peak brightness. Netflix, Disney+, and 
              broadcast HDR content use this transfer function for maximum 
              perceptual efficiency across the entire luminance range.
            </>
          )}
          {selectedCurve === 'hlg_rec2100' && (
            <>
              <strong>Hybrid Log-Gamma (HLG)</strong> - ITU-R BT.2100 transfer function 
              developed by BBC and NHK. Backward compatible with SDR displays while 
              supporting HDR. Used for live broadcast HDR content.
            </>
          )}
          {selectedCurve === 'gamma22' && (
            <>
              <strong>Gamma 2.2</strong> - Standard transfer function for sRGB and 
              Rec.709 (HDTV). Simple power function providing basic perceptual 
              uniformity for SDR content.
            </>
          )}
          {selectedCurve === 'gamma24' && (
            <>
              <strong>Gamma 2.4</strong> - DCI-P3 cinema standard with slightly 
              higher contrast than sRGB. Used in digital cinema projection.
            </>
          )}
          {selectedCurve === 'linear' && (
            <>
              <strong>Linear (Identity)</strong> - No transform applied. Direct 
              1:1 mapping between input and output. Useful as baseline reference.
            </>
          )}
        </p>
      </div>
      
      {/* Technical Specs */}
      {selectedCurve === 'pq_st2084' && (
        <div style={{
          marginTop: '12px',
          display: 'grid',
          gridTemplateColumns: 'repeat(4, 1fr)',
          gap: '8px',
        }}>
          {[
            { label: 'Peak Luminance', value: '10,000 nits' },
            { label: 'Min Luminance', value: '0.0001 nits' },
            { label: 'Bit Depth', value: '10/12-bit' },
            { label: 'Dynamic Range', value: '~42 stops' },
          ].map((spec) => (
            <div 
              key={spec.label}
              style={{
                padding: '8px',
                background: 'var(--bg-tertiary, #0d0d1a)',
                borderRadius: '4px',
                textAlign: 'center',
              }}
            >
              <div style={{ color: 'rgba(255,255,255,0.5)', fontSize: '10px' }}>
                {spec.label}
              </div>
              <div style={{ color: curveColors[selectedCurve], fontSize: '12px', fontWeight: 600 }}>
                {spec.value}
              </div>
            </div>
          ))}
        </div>
      )}
    </div>
  );
};

export default PQCurvesVisualization;
