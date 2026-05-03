import sys
import numpy as np
import matplotlib.pyplot as plt
from scipy.cluster.hierarchy import dendrogram

def main():
    if len(sys.argv) < 3:
        print("Usage: python draw_dendrogram.py <linkage.csv> <output.png>")
        return
        
    csv_file = sys.argv[1]
    out_file = sys.argv[2]
    
    try:
        Z = np.loadtxt(csv_file, delimiter=',')
        
        plt.figure(figsize=(10, 6))
        plt.title('Agglomerative Clustering Dendrogram')
        plt.xlabel('Sample index')
        plt.ylabel('Distance')
        
        # Don't show labels if there are too many leaves
        truncate_mode = 'level' if len(Z) > 100 else None
        p = 5 if len(Z) > 100 else 30
        
        dendrogram(
            Z,
            truncate_mode=truncate_mode,
            p=p,
            leaf_rotation=90.,
            leaf_font_size=8.,
            show_contracted=True
        )
        
        plt.tight_layout()
        plt.savefig(out_file, dpi=100)
        print(f"Dendrogram saved to {out_file}")
    except Exception as e:
        print(f"Error drawing dendrogram: {e}")
        sys.exit(1)

if __name__ == "__main__":
    main()
