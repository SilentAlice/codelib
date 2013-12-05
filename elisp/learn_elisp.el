(require 'helm)

(defun helm-myag ()
  (interactive)
  (helm :buffer "*helm-myag*"))

(provide 'helm-myag)

(defun func-with-opt-arg (fff &optional arg)
  (and (= fff 1) (eq arg nil)))
(func-with-opt-arg 1)

(defun* example (&rest args
                 &key (a 1) (b 2)
		 &allow-other-keys)
  (format "%s %s %s" args a b))
(example :a 10)
(example 10)

(defun* remote-path (&rest path
                     &key (user "jared") (server "someserver")
                     &allow-other-keys)
  (concat "ftp:" user "@" server ":"
          (mapconcat #'identity path "/")))

(remote-path :user "uat-user" "/tmp" "test-dir")

(defun remove-keyword-params (seq)
  (if (null seq) nil
    (let ((head (car seq))
          (tail (cdr seq)))
      (if (keywordp head) (remove-keyword-params (cdr tail))
        (cons head (remove-keyword-params tail))))))
(defun* remote-path (&rest path
                     &key (user "jared") (server "someserver")
                     &allow-other-keys)
  (concat "ftp:" user "@" server ":"
          (mapconcat #'identity (remove-keyword-params path) "/")))
