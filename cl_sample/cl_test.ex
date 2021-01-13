defmodule CLTest do
  @on_load :load_nifs

  def load_nifs do
    :erlang.load_nif('./cl_test', 0)
  end

  def cl_test(_a) do
    raise "NIF cl_test/1 not implemented"
  end
end
