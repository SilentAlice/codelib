(ns my-proj.core)

(require 'clojure.tools.trace)

;; naive implementation
(defn ^:dynamic qsort [array]
  ;; quick sort
  ;; dynamic is for trace
  (defn part [a pivot cmp]
    ;; part 
    (if (empty? a)
      []
      (if (cmp (last a) pivot)
        (conj (part (pop a) pivot cmp) (last a))
        (part (pop a) pivot cmp))))

  (if (empty? array)
    []
    (concat (qsort (part (pop array) (last array) <=))
            (list (last array))
            (qsort (part (pop array) (last array) >)))))

(qsort (map (fn [arg1] (rand-int 100)) (range 100)))

;; map is lazy-seq, make it eager
(qsort (vec (map (fn [arg1] (rand-int 100)) (range 100))))

(clojure.tools.trace/dotrace [qsort] (qsort [4 1 3 8 2 5 6]))

;; multithread version
(defn ^:dynamic qsort [array]
  ;; quick sort
  ;; dynamic is for trace
  (def remaining (if (empty? array)
                   []
                   (pop array)))

  (defn part [a pivot cmp]
    ;; part 
    (if (empty? a)
      []
      (if (cmp (last a) pivot)
        (conj (part (pop a) pivot cmp) (last a))
        (part (pop a) pivot cmp))))

  (println remaining)
  (if (empty? array)
    []
    (concat (.start (Thread. 
                     (qsort (part remaining (last array) <=))))
            (list (last array))
            (.start (Thread. 
                     (println array)
                     (qsort (part remaining (last array) >)))))))

;; (.start (Thread. (qsort (vec (map (fn [arg1] (rand-int 100)) (range 100))))))
(clojure.tools.trace/dotrace [qsort] (qsort [4 1 3 8 2 5 6]))
